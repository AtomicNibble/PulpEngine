#include "stdafx.h"
#include "XVideoSys.h"
#include "XVideo.h"

#include <Threading\JobSystem2.h>
#include <String\AssetName.h>
#include <IConsole.h>

#include <I3DEngine.h>

X_NAMESPACE_DECLARE(engine,
    class IPrimativeContext);

X_NAMESPACE_BEGIN(video)

// -----------------------------------------------

XVideoSys::XVideoSys(core::MemoryArenaBase* arena) :
    arena_(arena),
    blockArena_(arena),
    videos_(arena, sizeof(VideoResource), X_ALIGN_OF(VideoResource), "VideoPool"),
    pendingRequests_(arena)
{
}

void XVideoSys::registerVars(void)
{
    vars_.RegisterVars();
}

void XVideoSys::registerCmds(void)
{
    ADD_COMMAND_MEMBER("listVideos", this, XVideoSys, &XVideoSys::Cmd_ListVideo, core::VarFlag::SYSTEM,
        "List all the loaded videos");

    ADD_COMMAND_MEMBER("video_pause_all", this, XVideoSys, &XVideoSys::Cmd_PauseAll, core::VarFlag::SYSTEM,
        "Pause all the playing videos");
    ADD_COMMAND_MEMBER("video_resume_all", this, XVideoSys, &XVideoSys::Cmd_ResumeAll, core::VarFlag::SYSTEM,
        "Resume all the paused videos");

}

bool XVideoSys::init(void)
{
#if 0
    auto* pVideo = loadVideo("test/doom_trailer_720");

    waitForLoad(pVideo);
#endif

    return true;
}

void XVideoSys::shutDown(void)
{
    X_LOG0("Video", "Shutting Down");

    freeDangling();
}

void XVideoSys::release(void)
{
    X_DELETE(this, g_VideoArena);
}

void XVideoSys::update(const core::FrameTimeData& frameTimeInfo)
{
    core::ScopedLock<VideoContainer::ThreadPolicy> lock(videos_.getThreadPolicy());

    for (const auto& video : videos_) {
        auto* pVideoRes = video.second;

        pVideoRes->update(frameTimeInfo);
    }
}

void XVideoSys::appendDirtyBuffers(render::CommandBucket<uint32_t>& bucket) const
{
    core::ScopedLock<VideoContainer::ThreadPolicy> lock(videos_.getThreadPolicy());

    for (const auto& video : videos_) {
        auto* pVideoRes = video.second;

        if (pVideoRes->hasFrame()) {
            pVideoRes->appendDirtyBuffers(bucket);
        }
    }
}

void XVideoSys::unlockBuffers(void)
{
    core::ScopedLock<VideoContainer::ThreadPolicy> lock(videos_.getThreadPolicy());

    for (const auto& video : videos_) {
        auto* pVideoRes = video.second;

        pVideoRes->releaseFrame();
    }
}

IVideo* XVideoSys::findVideo(core::string_view name) const
{
    core::ScopedLock<VideoContainer::ThreadPolicy> lock(videos_.getThreadPolicy());

    auto* pVideo = videos_.findAsset(name);
    if (pVideo) {
        return pVideo;
    }

    X_WARNING("VidManager", "Failed to find video: \"%*.s\"", name.length(), name.data());
    return nullptr;
}

IVideo* XVideoSys::loadVideo(core::string_view name)
{
    X_ASSERT(core::strUtil::FileExtension(name) == nullptr, "Extension not allowed")(name);

    core::ScopedLock<VideoContainer::ThreadPolicy> lock(videos_.getThreadPolicy());

    VideoResource* pVideoRes = videos_.findAsset(name);
    if (pVideoRes) {
        pVideoRes->addReference();
        return pVideoRes;
    }
    
    pVideoRes = videos_.createAsset(name, vars_, blockArena_);

    queueLoadRequest(pVideoRes);

    return pVideoRes;
}

void XVideoSys::releaseVideo(IVideo* pVideo)
{
    VideoResource* pVideoRes = static_cast<VideoResource*>(pVideo);
    if (pVideoRes->removeReference() == 0) {
        videos_.releaseAsset(pVideoRes);
    }
}

bool XVideoSys::waitForLoad(core::AssetBase* pVideo)
{
    X_ASSERT(pVideo->getType() == assetDb::AssetType::VIDEO, "Invalid asset passed")();

    if (pVideo->isLoaded()) {
        return true;
    }

    return waitForLoad(static_cast<IVideo*>(static_cast<Video*>(pVideo)));
}

bool XVideoSys::waitForLoad(IVideo* pIVideo)
{
    Video* pVideo = static_cast<Video*>(pIVideo);

    {
        // we lock to see if loading as the setting of loading is performed inside this lock.
        core::CriticalSection::ScopedLock lock(loadReqLock_);
        while (pVideo->getStatus() == core::LoadStatus::Loading) {
            loadCond_.Wait(loadReqLock_);
        }
    }

    // did we fail? or never sent a dispatch?
    auto status = pVideo->getStatus();
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

void XVideoSys::freeDangling(void)
{
    {
        core::ScopedLock<VideoContainer::ThreadPolicy> lock(videos_.getThreadPolicy());

        // any left?
        for (const auto& m : videos_) {
            auto* pVideoRes = m.second;
            const auto& name = pVideoRes->getName();
            X_WARNING("Video", "\"%s\" was not deleted. refs: %" PRIi32, name.c_str(), pVideoRes->getRefCount());
        }
    }

    videos_.free();
}


void XVideoSys::queueLoadRequest(VideoResource* pVideoRes)
{
    // for video don't bother que, just dispatch now.

    pVideoRes->addReference(); // prevent instance sweep
    pVideoRes->setStatus(core::LoadStatus::Loading);

    core::CriticalSection::ScopedLock lock(loadReqLock_);

    auto loadReq = core::makeUnique<VideoLoadRequest>(arena_, blockArena_, pVideoRes);
    loadReq->buffer.resize(1024 * 16); // 16k baby!

    dispatchLoadRequest(loadReq.get());

    pendingRequests_.emplace_back(loadReq.release());
}

void XVideoSys::dispatchLoadRequest(VideoLoadRequest* pLoadReq)
{
    auto& name = pLoadReq->pVideo->getName();

    core::AssetName assetName(assetDb::AssetType::VIDEO, name, VIDEO_FILE_EXTENSION);

    core::IoRequestOpen open;
    open.callback.Bind<XVideoSys, &XVideoSys::IoRequestCallback>(this);
    open.pUserData = pLoadReq;
    open.mode = core::FileFlag::READ;
    open.path.set(assetName.begin(), assetName.end());

    gEnv->pFileSys->AddIoRequestToQue(open);
}

void XVideoSys::onLoadRequestFail(VideoLoadRequest* pLoadReq)
{
    auto* pVideo = pLoadReq->pVideo;

    pVideo->setStatus(core::LoadStatus::Error);

    // we only close file on error, we keep it open for streaming.
    if (pLoadReq->pFile) {
        gEnv->pFileSys->AddCloseRequestToQue(pLoadReq->pFile);
    }

    loadRequestCleanup(pLoadReq);
}

void XVideoSys::loadRequestCleanup(VideoLoadRequest* pLoadReq)
{
    {
        core::CriticalSection::ScopedLock lock(loadReqLock_);
        pendingRequests_.remove(pLoadReq);
    }

    // release our ref.
    releaseVideo(pLoadReq->pVideo);

    X_DELETE(pLoadReq, arena_);

    loadCond_.NotifyAll();
}

void XVideoSys::IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
    core::XFileAsync* pFile, uint32_t bytesTransferred)
{
    core::IoRequest::Enum requestType = pRequest->getType();
    VideoLoadRequest* pLoadReq = pRequest->getUserData<VideoLoadRequest>();
    auto* pVideo = pLoadReq->pVideo;

    if (requestType == core::IoRequest::OPEN) {
        if (!pFile) {
            X_ERROR("Video", "Failed to load: \"%s\"", pVideo->getName().c_str());
            onLoadRequestFail(pLoadReq);
            return;
        }

        pLoadReq->pFile = pFile;

        auto fileSize = pFile->fileSize();
        auto readSize = core::Min(pLoadReq->buffer.size(), static_cast<size_t>(fileSize));

        // should not result in allocation.
        pLoadReq->buffer.resize(readSize);

        // read the header.
        core::IoRequestRead read;
        read.callback.Bind<XVideoSys, &XVideoSys::IoRequestCallback>(this);
        read.pUserData = pLoadReq;
        read.pFile = pFile;
        read.dataSize = safe_static_cast<uint32_t>(pLoadReq->buffer.size());
        read.offset = 0;
        read.pBuf = pLoadReq->buffer.data();
        fileSys.AddIoRequestToQue(read);
    }
    else if (requestType == core::IoRequest::READ) {
        const core::IoRequestRead* pReadReq = static_cast<const core::IoRequestRead*>(pRequest);

        if (pReadReq->pBuf != pLoadReq->buffer.data()) {
            X_ASSERT_UNREACHABLE();
        }

        if (bytesTransferred != pLoadReq->buffer.size()) {
            X_ERROR("Video", "Failed to read video header. Got: 0x%" PRIx32 " need: 0x%" PRIxS, bytesTransferred, pLoadReq->buffer.size());
            onLoadRequestFail(pLoadReq);
            return;
        }

        gEnv->pJobSys->CreateMemberJobAndRun<XVideoSys>(this, &XVideoSys::processData_job,
            pLoadReq JOB_SYS_SUB_ARG(core::profiler::SubSys::VIDEO));
    }
    else {
        X_ASSERT_UNREACHABLE();
    }
}

void XVideoSys::processData_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
    X_UNUSED(jobSys, threadIdx, pJob, pData);

    auto* pLoadReq = static_cast<VideoLoadRequest*>(X_ASSERT_NOT_NULL(pData));
    auto* pVideo = pLoadReq->pVideo;

    if (pVideo->processHdr(pLoadReq->pFile, core::make_span(pLoadReq->buffer))) {
        pVideo->setStatus(core::LoadStatus::Complete);
    }
    else {
        pVideo->setStatus(core::LoadStatus::Error);
    }

    pVideo->play();

    loadRequestCleanup(pLoadReq);
}

void XVideoSys::listVideos(const char* pSearchPatten) const
{
    core::ScopedLock<VideoContainer::ThreadPolicy> lock(videos_.getThreadPolicy());

    core::Array<VideoResource*> sorted_videos(arena_);
    videos_.getSortedAssertList(sorted_videos, core::string_view(pSearchPatten));

    X_LOG0("Video", "------------ ^8Videos(%" PRIuS ")^7 ---------------", sorted_videos.size());

    core::HumanSize::Str sizeStr;
    for (const auto* pVideo : sorted_videos) {
        X_LOG0("Video", "^2%-32s^7 Dim: ^2%" PRIu16 "x%" PRIu16 "^7 Fps:^2%" PRIu32 "^7"
                        "^7 BufferSize: ^2%s^7 State: ^2%s^7 Refs:^2%" PRIi32,
            pVideo->getName(), pVideo->getWidth(), pVideo->getHeight(), pVideo->getFps(),
            core::HumanSize::toString(sizeStr, pVideo->getIOBufferSize()),
            State::ToString(pVideo->getState()),
            pVideo->getRefCount());
    }

    X_LOG0("Video", "------------ ^8Videos End^7 --------------");
}


void XVideoSys::pauseAll(void)
{
    core::ScopedLock<VideoContainer::ThreadPolicy> lock(videos_.getThreadPolicy());

    for (const auto& video : videos_) {
        auto* pVideoRes = video.second;

        pVideoRes->pause();
    }
}

void XVideoSys::resumeAll(void)
{
    core::ScopedLock<VideoContainer::ThreadPolicy> lock(videos_.getThreadPolicy());

    for (const auto& video : videos_) {
        auto* pVideoRes = video.second;

        auto state = pVideoRes->getState();
        if (state == State::Paused) {
            pVideoRes->play();
        }
    }
}


Vec2f XVideoSys::drawDebug(engine::IPrimativeContext* pPrim, Vec2f pos) const
{
    if (!vars_.drawDebug()) {
        return Vec2f();
    }

    Vec2f size;
    const float padding = 10.f;

    core::ScopedLock<VideoContainer::ThreadPolicy> lock(videos_.getThreadPolicy());

    for (const auto& video : videos_) {
        auto* pVideoRes = video.second;
        if (!pVideoRes->isLoaded()) {
            continue;
        }

        size += pVideoRes->drawDebug(pPrim, pos + size);
        size.y += padding;
    }

    return size;
}

void XVideoSys::Cmd_ListVideo(core::IConsoleCmdArgs* pCmd)
{
    // optional search criteria
    const char* pSearchPatten = nullptr;

    if (pCmd->GetArgCount() >= 2) {
        pSearchPatten = pCmd->GetArg(1);
    }

    listVideos(pSearchPatten);
}


void XVideoSys::Cmd_PauseAll(core::IConsoleCmdArgs* pCmd)
{
    X_UNUSED(pCmd);

    pauseAll();
}

void XVideoSys::Cmd_ResumeAll(core::IConsoleCmdArgs* pCmd)
{
    X_UNUSED(pCmd);

    resumeAll();
}

X_NAMESPACE_END