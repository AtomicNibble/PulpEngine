#include "EngineCommon.h"
#include "AssetLoader.h"
#include "AssetBase.h"
#include <String\AssetName.h>
#include <String\HumanDuration.h>
#include <Threading\ThreadLocalStorage.h>

#include <ITimer.h>
#include <IConsole.h>
#include <IFileSys.h>
#include <Threading\JobSystem2.h>

X_NAMESPACE_BEGIN(core)

AssetLoaderVars::AssetLoaderVars()
{
    debug_ = 0;
    maxActiveRequests_ = 16;
}

void AssetLoaderVars::registerVars(void)
{
    ADD_CVAR_REF("ldr_debug", debug_, debug_, 0, 2, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Enable AssetLoader debug");

    ADD_CVAR_REF("ldr_max_active_requests", maxActiveRequests_, maxActiveRequests_, 0, 1 << 10, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Max active load requests");

}

X_INLINE int32_t AssetLoaderVars::debugLvl(void) const
{
    return debug_;
}

X_INLINE int32_t AssetLoaderVars::maxActiveRequests(void) const
{
    return maxActiveRequests_;
}

// ------------------------------------------------------

AssetLoader::AssetLoader(core::MemoryArenaBase* arena, core::MemoryArenaBase* blockArena) :
    arena_(arena),
    blockArena_(blockArena),
    requestQueue_(arena),
    pendingRequests_(arena),
    pendingReloads_(arena)
{
    requestQueue_.reserve(64);
    pendingRequests_.setGranularity(32);

    assetsinks_.fill(nullptr);
    assetExt_.fill("");
}

void AssetLoader::registerVars(void)
{
    vars_.registerVars();
}

void AssetLoader::registerAssetType(assetDb::AssetType::Enum type, IAssetLoadSink* pSink, const char* pExt)
{
    X_ASSERT(assetsinks_[type] == nullptr, "Asset type already had a registered handler")(assetDb::AssetType::ToString(type));

    assetsinks_[type] = pSink;
    assetExt_[type] = pExt;
}

bool AssetLoader::onFileChanged(const char* pName)
{
    core::AssetName assetName(pName);
    assetName.replaceSeprators();

    // must be in a asset folder.
    auto* pSlash = assetName.find(core::AssetName::ASSET_NAME_SLASH);
    if (!pSlash) {
        return false;
    }

    core::StackString<128> assetTypeStr(assetName.begin(), pSlash);
    assetTypeStr.toLower();
    assetTypeStr.trimRight('s');

    assetDb::AssetType::Enum type;
    if (!assetDb::assetTypeFromStr(type, assetTypeStr.begin(), assetTypeStr.end())) {
        X_LOG0("AssetLoader", "File is not in asset folder: \"%s\"", assetName.c_str());
        return false;
    }

    // so we have the type!
    if (!assetsinks_[type]) {
        X_LOG0("AssetLoader", "No asset sink for: \"%s\"", assetDb::AssetType::ToString(type));
        return false;
    }

    core::AssetName tmp(assetName);
    tmp.replaceSeprators();
    tmp.stripAssetFolder(type);
    tmp.removeExtension();

    core::string nameStr(tmp.begin(), tmp.end());

    assetsinks_[type]->onFileChanged(assetName, nameStr);
    return true;
}

void AssetLoader::update(void)
{
    if (pendingReloads_.isNotEmpty()) {
        core::CriticalSection::ScopedLock lock(loadReqLock_);

        X_LOG1("AssetLoader", "Processing %" PRIuS " reload requests", pendingReloads_.size());

        // process them all.
        for (auto* pLoadRequest : pendingReloads_) {
            X_ASSERT(pLoadRequest->reloadFlags.IsSet(ReloadFlag::Beginframe), "Invalid reload flags")();
            X_ASSERT(!pLoadRequest->reloadFlags.IsSet(ReloadFlag::AnyTime), "Invalid reload flags")();

            processData(pLoadRequest);
        }

        pendingReloads_.clear();
    }
}

void AssetLoader::reload(AssetBase* pAsset, ReloadFlags flags)
{
    core::CriticalSection::ScopedLock lock(loadReqLock_);

    auto loadReq = core::makeUnique<AssetLoadRequest>(arena_, pAsset);

    loadReq->flags.Set(LoadFlag::Reload);
    loadReq->reloadFlags = flags;

    X_LOG0_IF(vars_.debugLvl(), "AssetLoader", "Reloading: ^4%s^7 -> \"%s\"",
        assetDb::AssetType::ToString(pAsset->getType()), pAsset->getName().c_str());

    // dispatch IO
    dispatchLoadRequest(loadReq.get());

    pendingRequests_.emplace_back(loadReq.release());
}

void AssetLoader::addLoadRequest(AssetBase* pAsset)
{
    X_ASSERT(assetsinks_[pAsset->getType()], "Asset type doest not have a registered handler")(assetDb::AssetType::ToString(pAsset->getType()));
    X_ASSERT(pAsset->getName().isNotEmpty(), "Asset name is empty")(assetDb::AssetType::ToString(pAsset->getType()));

    X_LOG0_IF(vars_.debugLvl() > 1, "AssetLoader", "Recived request: ^4%s^7 -> \"%s\"",
        assetDb::AssetType::ToString(pAsset->getType()), pAsset->getName().c_str());

    core::CriticalSection::ScopedLock lock(loadReqLock_);

    // if you are already loaded, return.
    auto status = pAsset->getStatus();
    if (status == core::LoadStatus::Complete || status == core::LoadStatus::Loading) {
        X_WARNING("AssetLoader", "Redundant load request requested: \"%s\"", pAsset->getName().c_str());
        return;
    }

    //	pAsset->addReference(); // prevent instance sweep
    pAsset->setStatus(core::LoadStatus::Loading);

    const int32_t maxReq = vars_.maxActiveRequests();
    if (maxReq == 0 || safe_static_cast<int32_t>(pendingRequests_.size()) < maxReq) {
        dispatchLoad(pAsset, lock);
    }
    else {
        queueLoadRequest(pAsset, lock);
    }
}

bool AssetLoader::waitForLoad(AssetBase* pAsset)
{
#if X_ENABLE_ASSET_LOADER_DEADLOCK_CHECK
    X_ASSERT(processingThreads_.getValueInt() == 0, "Can't waitForLoad inside a asset data job, can deadlock")(processingThreads_.getValueInt());
#endif // !X_ENABLE_ASSET_LOADER_DEADLOCK_CHECK

    if (pAsset->getStatus() == core::LoadStatus::Complete) {
        return true;
    }

    {
        // TODO: work out if this lock is really required.
        core::CriticalSection::ScopedLock lock(loadReqLock_);
        while (pAsset->getStatus() == core::LoadStatus::Loading) {
            dispatchPendingLoads();

            loadCond_.Wait(loadReqLock_);
        }
    }

    // did we fail? or never sent a dispatch?
    auto status = pAsset->getStatus();
    if (status == core::LoadStatus::Complete) {
        return true;
    }
    else if (status == core::LoadStatus::Error) {
        return false;
    }
    else if (status == core::LoadStatus::NotLoaded) {
        // request never sent?
    }

    X_ASSERT_UNREACHABLE();
    return false;
}

void AssetLoader::dispatchPendingLoads(void)
{
    core::CriticalSection::ScopedLock lock(loadReqLock_);

    while (dispatchPendingLoad(lock)) {
        // ...
    }
}

void AssetLoader::queueLoadRequest(AssetBase* pAsset, core::CriticalSection::ScopedLock&)
{
    X_ASSERT(!requestQueue_.contains(pAsset), "Queue already contains asset")(pAsset);

    requestQueue_.push(pAsset);
}

void AssetLoader::dispatchLoad(AssetBase* pAsset, core::CriticalSection::ScopedLock&)
{
    X_ASSERT(pAsset->getStatus() == core::LoadStatus::Loading || pAsset->getStatus() == core::LoadStatus::NotLoaded, "Incorrect status")();

    auto loadReq = core::makeUnique<AssetLoadRequest>(arena_, pAsset);

    // dispatch IO
    dispatchLoadRequest(loadReq.get());

    pendingRequests_.emplace_back(loadReq.release());
}

bool AssetLoader::dispatchPendingLoad(core::CriticalSection::ScopedLock& lock)
{
    const int32_t maxReq = vars_.maxActiveRequests();

    if (requestQueue_.isNotEmpty() && (maxReq == 0 || safe_static_cast<int32_t>(pendingRequests_.size()) < maxReq)) {
        X_ASSERT(requestQueue_.peek()->getStatus() == core::LoadStatus::Loading, "Incorrect status")();
        dispatchLoad(requestQueue_.peek(), lock);
        requestQueue_.pop();
        return true;
    }

    return false;
}

void AssetLoader::dispatchLoadRequest(AssetLoadRequest* pLoadReq)
{
    auto* pAsset = pLoadReq->pAsset;
    auto type = pAsset->getType();
    auto& name = pLoadReq->pAsset->getName();

    X_LOG0_IF(vars_.debugLvl(), "AssetLoader", "Dispatching: ^4%s^7 -> \"%s\"",
        assetDb::AssetType::ToString(type), name.c_str());

    pLoadReq->dispatchTime = gEnv->pTimer->GetTimeNowReal();

    core::AssetName assetName(pAsset->getType(), name, assetExt_[type]);

    // dispatch a read request baby!
    core::IoRequestOpen open;
    open.callback.Bind<AssetLoader, &AssetLoader::IoRequestCallback>(this);
    open.pUserData = pLoadReq;
    open.mode = core::fileMode::READ | core::fileMode::SHARE;
    open.path.set(assetName.begin(), assetName.end());

    gEnv->pFileSys->AddIoRequestToQue(open);
}

void AssetLoader::onLoadRequestFail(AssetLoadRequest* pLoadReq)
{
    auto* pAsset = pLoadReq->pAsset;
    auto type = pAsset->getType();

    pAsset->setStatus(core::LoadStatus::Error);

    assetsinks_[type]->onLoadRequestFail(pAsset);

    loadRequestCleanup(pLoadReq);
}

void AssetLoader::onLoadRequestSuccess(AssetLoadRequest* pLoadReq)
{
    auto* pAsset = pLoadReq->pAsset;

    pAsset->setStatus(core::LoadStatus::Complete);

    if (vars_.debugLvl() > 1)
    {
        auto now = gEnv->pTimer->GetTimeNowReal();
        auto loadTime = pLoadReq->ioCompleteTime - pLoadReq->dispatchTime;
        auto processDelay = pLoadReq->processBegin - pLoadReq->ioCompleteTime;
        auto processTime = now - pLoadReq->processBegin;
        auto totalTime = now - pLoadReq->dispatchTime;

        core::HumanDuration::Str durStr0, durStr1, durStr2, durStr3;

        X_LOG0("assetLoader", "^4%s^7 -> \"%s\" IO ^6%s^7 Delay ^6%s^7 Process ^6%s^7 Total ^6%s", assetDb::AssetType::ToString(pAsset->getType()),
            pAsset->getName().c_str(), core::HumanDuration::toString(durStr0, loadTime.GetMilliSeconds()),
            core::HumanDuration::toString(durStr1, processDelay.GetMilliSeconds()),
            core::HumanDuration::toString(durStr2, processTime.GetMilliSeconds()),
            core::HumanDuration::toString(durStr3, totalTime.GetMilliSeconds())
        );
    }

    loadRequestCleanup(pLoadReq);
}

void AssetLoader::loadRequestCleanup(AssetLoadRequest* pLoadReq)
{
    auto status = pLoadReq->pAsset->getStatus();
    X_ASSERT(status == core::LoadStatus::Complete || status == core::LoadStatus::Error, "Unexpected load status")(status);

    {
        core::CriticalSection::ScopedLock lock(loadReqLock_);
        pendingRequests_.remove(pLoadReq);
    }

    if (pLoadReq->pFile) {
        gEnv->pFileSys->AddCloseRequestToQue(pLoadReq->pFile);
    }

    // release our ref.
    //releaseScript(static_cast<Script*>(pLoadReq->pAsset));

    X_DELETE(pLoadReq, arena_);

    loadCond_.NotifyAll();
}

void AssetLoader::IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
    core::XFileAsync* pFile, uint32_t bytesTransferred)
{
    core::IoRequest::Enum requestType = pRequest->getType();
    AssetLoadRequest* pLoadReq = pRequest->getUserData<AssetLoadRequest>();
    auto* pAsset = pLoadReq->pAsset;

    if (requestType == core::IoRequest::OPEN) {
        if (!pFile) {
            X_ERROR("AssetLoader", "Failed to load: ^4%s^7 -> \"%s\"",
                assetDb::AssetType::ToString(pLoadReq->pAsset->getType()), pAsset->getName().c_str());
            onLoadRequestFail(pLoadReq);
            return;
        }

        pLoadReq->pFile = pFile;

        // read the whole file.
        uint32_t fileSize = safe_static_cast<uint32_t>(pFile->fileSize());
        X_ASSERT(fileSize > 0, "Datasize must be positive")(fileSize);
        pLoadReq->data = core::makeUnique<char[]>(blockArena_, fileSize, 16);
        pLoadReq->dataSize = fileSize;

        core::IoRequestRead read;
        read.callback.Bind<AssetLoader, &AssetLoader::IoRequestCallback>(this);
        read.pUserData = pLoadReq;
        read.pFile = pFile;
        read.dataSize = fileSize;
        read.offset = 0;
        read.pBuf = pLoadReq->data.ptr();
        fileSys.AddIoRequestToQue(read);
    }
    else if (requestType == core::IoRequest::READ) {
        const core::IoRequestRead* pReadReq = static_cast<const core::IoRequestRead*>(pRequest);

        X_ASSERT(pLoadReq->data.ptr() == pReadReq->pBuf, "Buffers don't match")();

        if (bytesTransferred != pReadReq->dataSize) {
            X_ERROR("AssetLoader", "Failed to read asset data. Got: 0x%" PRIx32 " need: 0x%" PRIx32, bytesTransferred, pReadReq->dataSize);
            onLoadRequestFail(pLoadReq);
            return;
        }

        pLoadReq->ioCompleteTime = gEnv->pTimer->GetTimeNowReal();

        // hello my sexy reload!
        if (pLoadReq->flags.IsSet(LoadFlag::Reload)) {
            if (!pLoadReq->reloadFlags.IsSet(ReloadFlag::AnyTime)) {
                core::CriticalSection::ScopedLock lock(loadReqLock_);
                pendingReloads_.push_back(pLoadReq);
                return;
            }
        }

        gEnv->pJobSys->CreateMemberJobAndRun<AssetLoader>(this, &AssetLoader::processData_job,
            pLoadReq JOB_SYS_SUB_ARG(core::profiler::SubSys::CORE));
    }
}

void AssetLoader::processData_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
    X_UNUSED(jobSys, threadIdx, pJob, pData);

    auto* pLoadReq = static_cast<AssetLoadRequest*>(X_ASSERT_NOT_NULL(pData));

    processData(pLoadReq);
}

void AssetLoader::processData(AssetLoadRequest* pRequest)
{
    auto* pAsset = pRequest->pAsset;
    auto type = pAsset->getType();

    pRequest->processBegin = gEnv->pTimer->GetTimeNowReal();

#if X_ENABLE_ASSET_LOADER_DEADLOCK_CHECK
    processingThreads_.setValueInt(1);
#endif // !X_ENABLE_ASSET_LOADER_DEADLOCK_CHECK

    auto ok = assetsinks_[type]->processData(pAsset, std::move(pRequest->data), pRequest->dataSize);

#if X_ENABLE_ASSET_LOADER_DEADLOCK_CHECK
    processingThreads_.setValueInt(0);
#endif // !X_ENABLE_ASSET_LOADER_DEADLOCK_CHECK

    if (!ok) {
        onLoadRequestFail(pRequest);
        return;
    }

    onLoadRequestSuccess(pRequest);
}

X_NAMESPACE_END