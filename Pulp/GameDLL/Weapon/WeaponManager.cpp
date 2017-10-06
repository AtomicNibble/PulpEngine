#include "stdafx.h"
#include "WeaponManager.h"

#include <IFileSys.h>
#include <Threading\JobSystem2.h>

X_NAMESPACE_BEGIN(game)

namespace weapon
{

	WeaponDefManager::WeaponDefManager(core::MemoryArenaBase* arena) :
		arena_(arena),
		blockArena_(arena),
		pDefaultWeaponDef_(nullptr),
		pendingRequests_(arena),
		weaponDefs_(arena, sizeof(WeaponDefResource), X_ALIGN_OF(WeaponDefResource), "WeaponDefs")
	{

	}

	void WeaponDefManager::registerCmds(void)
	{

	}

	void WeaponDefManager::registerVars(void)
	{

	}

	bool WeaponDefManager::init(void)
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pHotReload);

		if (!initDefaults()) {
			return false;
		}

		gEnv->pHotReload->addfileType(this, WEAPON_FILE_EXTENSION);

		return true;
	}

	void WeaponDefManager::shutDown(void)
	{
		gEnv->pHotReload->unregisterListener(this);


		if (pDefaultWeaponDef_) {
			releaseWeaponDef(pDefaultWeaponDef_);
		}

		freeDangling();
	}

	bool WeaponDefManager::asyncInitFinalize(void)
	{
		if (!pDefaultWeaponDef_) {
			X_ERROR("WeaponDef", "Default WeaponDef is not valid");
			return false;
		}

		if (!waitForLoad(pDefaultWeaponDef_)) {
			X_ERROR("WeaponDef", "Failed to load default WeaponDef");
			return false;
		}

		return true;
	}

	WeaponDef* WeaponDefManager::findWeaponDef(const char* pName) const
	{
		core::string name(pName);

		core::ScopedLock<WeaponDefContainer::ThreadPolicy> lock(weaponDefs_.getThreadPolicy());

		WeaponDef* pDef = weaponDefs_.findAsset(name);
		if (pDef) {
			return pDef;
		}

		X_WARNING("WeaponDef", "Failed to find material: \"%s\"", pName);
		return nullptr;
	}

	WeaponDef* WeaponDefManager::loadWeaponDef(const char* pName)
	{
		X_ASSERT(core::strUtil::FileExtension(pName) == nullptr, "Extension not allowed")(pName);

		core::string name(pName);
		WeaponDefResource* pWeaponDefRes = nullptr;

		core::ScopedLock<WeaponDefContainer::ThreadPolicy> lock(weaponDefs_.getThreadPolicy());

		{
			pWeaponDefRes = weaponDefs_.findAsset(name);
			if (pWeaponDefRes)
			{
				// inc ref count.
				pWeaponDefRes->addReference();
				return pWeaponDefRes;
			}

			pWeaponDefRes = weaponDefs_.createAsset(name, name);
		}

		addLoadRequest(pWeaponDefRes);

		return pWeaponDefRes;
	}

	WeaponDef* WeaponDefManager::getDefaultDef(void) const
	{
		X_ASSERT_NOT_IMPLEMENTED();
		return nullptr;
	}



	bool WeaponDefManager::waitForLoad(core::AssetBase* pWeaponDef)
	{
		X_ASSERT(pWeaponDef->getType() == assetDb::AssetType::WEAPON, "Invalid asset passed")();

		if (pWeaponDef->isLoaded()) {
			return true;
		}

		return waitForLoad(static_cast<WeaponDef*>(pWeaponDef));
	}


	bool WeaponDefManager::waitForLoad(WeaponDef* pWeaponDef)
	{
		if (pWeaponDef->getStatus() == core::LoadStatus::Complete) {
			return true;
		}

		{
			// we lock to see if loading as the setting of loading is performed inside this lock.
			core::CriticalSection::ScopedLock lock(loadReqLock_);
			while (pWeaponDef->getStatus() == core::LoadStatus::Loading)
			{
				loadCond_.Wait(loadReqLock_);
			}

		}

		// did we fail? or never sent a dispatch?
		auto status = pWeaponDef->getStatus();
		if (status == core::LoadStatus::Complete)
		{
			return true;
		}
		else if (status == core::LoadStatus::Error)
		{
			return false;
		}
		else if (status == core::LoadStatus::NotLoaded)
		{
			// request never sent?
		}

		X_ASSERT_UNREACHABLE();
		return false;
	}

	void WeaponDefManager::releaseWeaponDef(WeaponDef* pWeaponDef)
	{
		WeaponDefResource* pWeaponDefRes = reinterpret_cast<WeaponDefResource*>(pWeaponDef);
		if (pWeaponDefRes->removeReference() == 0)
		{
			weaponDefs_.releaseAsset(pWeaponDefRes);
		}
	}

	bool WeaponDefManager::initDefaults(void)
	{
		if (!pDefaultWeaponDef_)
		{
			pDefaultWeaponDef_ = loadWeaponDef(WEAPON_DEFAULT_NAME);
			if (!pDefaultWeaponDef_) {
				X_ERROR("WeaponDef", "Failed to create default weapondef");
				return false;
			}
		}

		return true;
	}

	void WeaponDefManager::freeDangling(void)
	{

	}

	void WeaponDefManager::addLoadRequest(WeaponDefResource* pWeaponDef)
	{
		core::CriticalSection::ScopedLock lock(loadReqLock_);

		pWeaponDef->addReference(); // prevent instance sweep
		pWeaponDef->setStatus(core::LoadStatus::Loading);

		dispatchLoad(pWeaponDef, lock);
	}

	void WeaponDefManager::dispatchLoad(WeaponDef* pWeaponDef, core::CriticalSection::ScopedLock&)
	{
		X_ASSERT(pWeaponDef->getStatus() == core::LoadStatus::Loading || pWeaponDef->getStatus() == core::LoadStatus::NotLoaded, "Incorrect status")();

		auto loadReq = core::makeUnique<WeaponDefLoadRequest>(arena_, pWeaponDef);

		// dispatch IO 
		dispatchLoadRequest(loadReq.get());

		pendingRequests_.emplace_back(loadReq.release());
	}

	void WeaponDefManager::dispatchLoadRequest(WeaponDefLoadRequest* pLoadReq)
	{
		core::Path<char> path;
		path /= "weapons";
		path /= pLoadReq->pWeaponDef->getName();
		path.setExtension(WEAPON_FILE_EXTENSION);

		// dispatch a read request baby!
		core::IoRequestOpen open;
		open.callback.Bind<WeaponDefManager, &WeaponDefManager::IoRequestCallback>(this);
		open.pUserData = pLoadReq;
		open.mode = core::fileMode::READ;
		open.path = path;

		gEnv->pFileSys->AddIoRequestToQue(open);
	}

	void WeaponDefManager::onLoadRequestFail(WeaponDefLoadRequest* pLoadReq)
	{
		auto* pWeaponDef = pLoadReq->pWeaponDef;

		if (pWeaponDef != pDefaultWeaponDef_)
		{
			if (!pDefaultWeaponDef_->isLoaded())
			{
				X_ASSERT_NOT_IMPLEMENTED();
			}
			else
			{
			//	pWeaponDef->assignProps(*pDefaultMtl_);
			}
		}
		else
		{
			pWeaponDef->setStatus(core::LoadStatus::Error);
		}


		loadRequestCleanup(pLoadReq);
	}

	void WeaponDefManager::loadRequestCleanup(WeaponDefLoadRequest* pLoadReq)
	{
		auto status = pLoadReq->pWeaponDef->getStatus();
		X_ASSERT(status == core::LoadStatus::Complete || status == core::LoadStatus::Error, "Unexpected load status")(status);

		{
			core::CriticalSection::ScopedLock lock(loadReqLock_);
			pendingRequests_.remove(pLoadReq);
		}

		if (pLoadReq->pFile) {
			gEnv->pFileSys->AddCloseRequestToQue(pLoadReq->pFile);
		}

		// release our ref.
		releaseWeaponDef(pLoadReq->pWeaponDef);

		X_DELETE(pLoadReq, arena_);

		loadCond_.NotifyAll();
	}

	void WeaponDefManager::IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
		core::XFileAsync* pFile, uint32_t bytesTransferred)
	{
		core::IoRequest::Enum requestType = pRequest->getType();
		WeaponDefLoadRequest* pLoadReq = pRequest->getUserData<WeaponDefLoadRequest>();
		WeaponDef* pWeaponDef = pLoadReq->pWeaponDef;

		if (requestType == core::IoRequest::OPEN)
		{
			if (!pFile)
			{
				X_ERROR("WeaponDef", "Failed to load: \"%s\"", pWeaponDef->getName().c_str());
				onLoadRequestFail(pLoadReq);
				return;
			}

			pLoadReq->pFile = pFile;

			// read the whole file.
			uint32_t fileSize = safe_static_cast<uint32_t>(pFile->fileSize());
			X_ASSERT(fileSize > 0, "Datasize must be positive")(fileSize);
			pLoadReq->data = core::makeUnique<uint8_t[]>(blockArena_, fileSize, 16);
			pLoadReq->dataSize = fileSize;

			core::IoRequestRead read;
			read.callback.Bind<WeaponDefManager, &WeaponDefManager::IoRequestCallback>(this);
			read.pUserData = pLoadReq;
			read.pFile = pFile;
			read.dataSize = fileSize;
			read.offset = 0;
			read.pBuf = pLoadReq->data.ptr();
			fileSys.AddIoRequestToQue(read);
		}
		else if (requestType == core::IoRequest::READ)
		{
			const core::IoRequestRead* pReadReq = static_cast<const core::IoRequestRead*>(pRequest);

			X_ASSERT(pLoadReq->data.ptr() == pReadReq->pBuf, "Buffers don't match")();

			if (bytesTransferred != pReadReq->dataSize)
			{
				X_ERROR("WeaponDef", "Failed to read WeaponDef data. Got: 0x%" PRIx32 " need: 0x%" PRIx32, bytesTransferred, pReadReq->dataSize);
				onLoadRequestFail(pLoadReq);
				return;
			}

			gEnv->pJobSys->CreateMemberJobAndRun<WeaponDefManager>(this, &WeaponDefManager::ProcessData_job,
				pLoadReq JOB_SYS_SUB_ARG(core::profiler::SubSys::GAME));
		}
	}

	void WeaponDefManager::ProcessData_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
	{
		X_UNUSED(jobSys, threadIdx, pJob, pData);

		WeaponDefLoadRequest* pLoadReq = static_cast<WeaponDefLoadRequest*>(X_ASSERT_NOT_NULL(pData));
		WeaponDef* pWeaponDef = pLoadReq->pWeaponDef;

		core::XFileFixedBuf file(pLoadReq->data.ptr(), pLoadReq->data.ptr() + pLoadReq->dataSize);

		if (!processData(pWeaponDef, &file))
		{
			onLoadRequestFail(pLoadReq);
		}
		else
		{
			loadRequestCleanup(pLoadReq);
		}
	}

	bool WeaponDefManager::processData(WeaponDef* pWeaponDef, core::XFile* pFile)
	{
		if (!pWeaponDef->processData(pFile)) {
			pWeaponDef->setStatus(core::LoadStatus::Error);
			return false;
		}

		pWeaponDef->setStatus(core::LoadStatus::Complete);
		return true;
	}

	void WeaponDefManager::Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name)
	{
		X_UNUSED(jobSys, name);


	}



} // namespace weapon

X_NAMESPACE_END