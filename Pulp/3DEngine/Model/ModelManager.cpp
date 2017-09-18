#include <stdafx.h>
#include <Model\ModelManager.h>

#include "EngineEnv.h"
#include "Material\MaterialManager.h"
#include "RenderModel.h"

#include <IConsole.h>
#include <IFileSys.h>
#include <Threading\JobSystem2.h>
#include <Time\StopWatch.h>

X_NAMESPACE_BEGIN(model)

XModelManager::XModelManager(core::MemoryArenaBase* arena, core::MemoryArenaBase* blockArena) :
	arena_(arena),
	blockArena_(blockArena),
	pDefaultModel_(nullptr),
	models_(arena, sizeof(ModelResource), X_ALIGN_OF(ModelResource), "ModelPool"),
	requestQueue_(arena),
	pendingRequests_(arena)
{
	requestQueue_.reserve(64);
	pendingRequests_.setGranularity(32);
}

XModelManager::~XModelManager()
{

}



void XModelManager::registerCmds(void)
{
	ADD_COMMAND_MEMBER("listModels", this, XModelManager, &XModelManager::Cmd_ListModels, core::VarFlag::SYSTEM, "List all the loaded models");
	ADD_COMMAND_MEMBER("modelReload", this, XModelManager, &XModelManager::Cmd_ReloadModel, core::VarFlag::SYSTEM, "Reload a model <name>");

}

void XModelManager::registerVars(void)
{

}

bool XModelManager::init(void)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pHotReload);


	if (!initDefaults()) {
		return false;
	}

	// hotreload support.
	gEnv->pHotReload->addfileType(this, MODEL_FILE_EXTENSION);

	return true;
}


void XModelManager::shutDown(void)
{
	X_LOG0("ModelManager", "Shutting Down");

	gEnv->pHotReload->unregisterListener(this);

	// default model
	if (pDefaultModel_) {
		releaseModel(pDefaultModel_);
	}

	freeDanglingMaterials();
}

bool XModelManager::asyncInitFinalize(void)
{
	if (!pDefaultModel_) {
		X_ERROR("ModelManager", "Default model is not valid");
		return false;
	}

	if (!waitForLoad(pDefaultModel_)) {
		X_ERROR("ModelManager", "Failed to load default model");
		return false;
	}
	
	return true;
}


XModel* XModelManager::findModel(const char* pModelName) const
{
	core::string name(pModelName);
	core::ScopedLock<ModelContainer::ThreadPolicy> lock(models_.getThreadPolicy());

	ModelResource* pModel = models_.findAsset(name);
	if (pModel) {
		return pModel;
	}

	X_WARNING("ModelManager", "Failed to find model: \"%s\"", pModelName);
	return nullptr;
}


XModel* XModelManager::loadModel(const char* pModelName)
{
	X_ASSERT_NOT_NULL(pModelName);
	X_ASSERT(core::strUtil::FileExtension(pModelName) == nullptr, "Extension not allowed")(pModelName);

	core::string name(pModelName);
	core::ScopedLock<ModelContainer::ThreadPolicy> lock(models_.getThreadPolicy());

	ModelResource* pModelRes = models_.findAsset(name);
	if (pModelRes)
	{
		// inc ref count.
		pModelRes->addReference();
		return pModelRes;
	}

	// we create a model and give it back
	pModelRes = models_.createAsset(name, name);

	// add to list of models that need loading.
	queueLoadRequest(pModelRes);

	return pModelRes;
}

XModel* XModelManager::getDefaultModel(void) const
{
	return pDefaultModel_;
}

void XModelManager::releaseModel(XModel* pModel)
{
	ModelResource* pModelRes = static_cast<ModelResource*>(pModel);
	if (pModelRes->removeReference() == 0)
	{
		releaseResources(pModelRes);

		models_.releaseAsset(pModelRes);
	}
}

bool XModelManager::initDefaults(void)
{
	pDefaultModel_ = static_cast<RenderModel*>(loadModel(MODEL_DEFAULT_NAME));
	if (!pDefaultModel_) {
		X_ERROR("ModelManager", "Failed to create default model");
		return false;
	}

	dispatchPendingLoads();

	return true;
}

void XModelManager::freeDanglingMaterials(void)
{
	{
		core::ScopedLock<ModelContainer::ThreadPolicy> lock(models_.getThreadPolicy());

		// any left?
		for (const auto& m : models_)
		{
			auto* pModelRes = m.second;
			const auto& name = pModelRes->getName();
			X_WARNING("XModel", "\"%s\" was not deleted. refs: %" PRIi32, name.c_str(), pModelRes->getRefCount());

			releaseResources(pModelRes);
		}
	}

	models_.free();
}

void XModelManager::releaseResources(XModel* pModel)
{
	X_UNUSED(pModel);

}

void XModelManager::reloadModel(const char* pModelName)
{
	core::string name(pModelName);

	ModelResource* pModelRes = nullptr;
	{
		core::ScopedLock<ModelContainer::ThreadPolicy> lock(models_.getThreadPolicy());
		pModelRes = models_.findAsset(name);
	}

	if (pModelRes)
	{
		X_LOG0("Model", "Reload model: \"%s\"", name.c_str());
		X_ASSERT_NOT_IMPLEMENTED();
	}
	else
	{
		X_WARNING("Model", "\"%s\" is not loaded skipping reload", name.c_str());
	}
}

void XModelManager::listModels(const char* pSearchPatten) const
{
	core::ScopedLock<ModelContainer::ThreadPolicy> lock(models_.getThreadPolicy());

	core::Array<ModelResource*> sorted_models(g_3dEngineArena);
	sorted_models.setGranularity(models_.size());

	for (const auto& model : models_)
	{
		auto* pModelRes = model.second;

		if (!pSearchPatten || core::strUtil::WildCompare(pSearchPatten, pModelRes->getName()))
		{
			sorted_models.push_back(pModelRes);
		}
	}

	std::sort(sorted_models.begin(), sorted_models.end(), [](ModelResource* a, ModelResource* b){
			const auto& nameA = a->getName();
			const auto& nameB = b->getName();
			return nameA.compareInt(nameB) < 0;
		}
	);

	X_LOG0("Model", "------------ ^8Models(%" PRIuS ")^7 ---------------", sorted_models.size());

	for (const auto* pModel : sorted_models)
	{
		X_LOG0("Model", "^2%-32s^7 Lods:^2%i^7 Bones:^2%i^7 RootBones:^2%i^7 TotalMesh:^2%i^7 Refs:^2%i",
			pModel->getName(), pModel->getNumLods(), pModel->getNumBones(), pModel->getNumRootBones(),
			pModel->getNumMeshTotal(), pModel->getRefCount());
	}

	X_LOG0("Model", "------------ ^8Models End^7 --------------");
}

void XModelManager::queueLoadRequest(ModelResource* pModel)
{
	X_ASSERT(pModel->getName().isNotEmpty(), "Model has no name")();

	core::CriticalSection::ScopedLock lock(loadReqLock_);

	// can we know it's not in this queue just from status?
	// like if it's complete it could be in this status
	auto status = pModel->getStatus();
	if (status == core::LoadStatus::Complete || status == core::LoadStatus::Loading)
	{
		X_WARNING("Model", "Redundant load request requested: \"%s\"", pModel->getName().c_str());
		return;
	}

	X_ASSERT(!requestQueue_.contains(pModel), "Queue already contains asset")(pModel);

	pModel->addReference(); // prevent instance sweep
	pModel->setStatus(core::LoadStatus::Loading);
	requestQueue_.push(pModel);
}


void XModelManager::dispatchPendingLoads(void)
{
	core::CriticalSection::ScopedLock lock(loadReqLock_);

	while (requestQueue_.isNotEmpty())
	{
		auto* pModel = requestQueue_.peek();
		X_ASSERT(pModel->getStatus() == core::LoadStatus::Loading, "Incorrect status")(pModel->getStatus());
		auto loadReq = core::makeUnique<ModelLoadRequest>(arena_, pModel);

		// dispatch IO 
		dispatchLoadRequest(loadReq.get());

		pendingRequests_.emplace_back(loadReq.release());

		requestQueue_.pop();
	}
}

bool XModelManager::waitForLoad(XModel* pModel)
{
	{
		// we lock to see if loading as the setting of loading is performed inside this lock.
		core::CriticalSection::ScopedLock lock(loadReqLock_);
		while (pModel->getStatus() == core::LoadStatus::Loading)
		{
			loadCond_.Wait(loadReqLock_);
		}
	}

	// did we fail? or never sent a dispatch?
	auto status = pModel->getStatus();
	if (status == core::LoadStatus::Complete)
	{
		return true;
	}
	else if(status == core::LoadStatus::Error)
	{
		return false;
	}
	else if(status == core::LoadStatus::NotLoaded)
	{
		// request never sent?
	}

	X_ASSERT_UNREACHABLE();
	return false;
}

void XModelManager::dispatchLoadRequest(ModelLoadRequest* pLoadReq)
{
	pLoadReq->dispatchTime = core::StopWatch::GetTimeNow();

	core::Path<char> path;
	path /= "models";
	path /= pLoadReq->pModel->getName();
	path.setExtension(".model");

	// dispatch a read request baby!
	core::IoRequestOpen open;
	open.callback.Bind<XModelManager, &XModelManager::IoRequestCallback>(this);
	open.pUserData = pLoadReq;
	open.mode = core::fileMode::READ;
	open.path = path;

	gEnv->pFileSys->AddIoRequestToQue(open);
}

void XModelManager::onLoadRequestFail(ModelLoadRequest* pLoadReq)
{
	auto* pModel = pLoadReq->pModel;

	if (pDefaultModel_ != pModel) 
	{
		// what if default model not loaded :| ?
		if (!pDefaultModel_->isLoaded())
		{
			waitForLoad(pDefaultModel_);
		}

		// only assing if valid.
		if (pDefaultModel_->isLoaded())
		{
			pModel->assignDefault(pDefaultModel_);
		}
	}

	pModel->setStatus(core::LoadStatus::Error);

	loadRequestCleanup(pLoadReq);
}

void XModelManager::loadRequestCleanup(ModelLoadRequest* pLoadReq)
{
	pLoadReq->loadTime = core::StopWatch::GetTimeNow();

	X_LOG0("Model", "Model loaded in: ^6%fms", (pLoadReq->loadTime - pLoadReq->dispatchTime).GetMilliSeconds());
	{
		core::CriticalSection::ScopedLock lock(loadReqLock_);
		pendingRequests_.remove(pLoadReq);
	}

	if (pLoadReq->pFile) {
		gEnv->pFileSys->AddCloseRequestToQue(pLoadReq->pFile);
	}

	// release our ref.
	releaseModel(pLoadReq->pModel);

	X_DELETE(pLoadReq, arena_);

	loadCond_.NotifyAll();
}

void XModelManager::IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
	core::XFileAsync* pFile, uint32_t bytesTransferred)
{
	core::IoRequest::Enum requestType = pRequest->getType();
	ModelLoadRequest* pLoadReq = pRequest->getUserData<ModelLoadRequest>();
	XModel* pModel = pLoadReq->pModel;

	if (requestType == core::IoRequest::OPEN)
	{
		if (!pFile) 
		{
			X_ERROR("Model", "Failed to load: \"%s\"", pModel->getName().c_str());
			onLoadRequestFail(pLoadReq);
			return;
		}

		pLoadReq->pFile = pFile;

		// read the header.
		core::IoRequestRead read;
		read.callback.Bind<XModelManager, &XModelManager::IoRequestCallback>(this);
		read.pUserData = pLoadReq;
		read.pFile = pFile;
		read.dataSize = sizeof(pLoadReq->hdr);
		read.offset = 0;
		read.pBuf = &pLoadReq->hdr;
		fileSys.AddIoRequestToQue(read);
	}
	else if (requestType == core::IoRequest::READ)
	{
		const core::IoRequestRead* pReadReq = static_cast<const core::IoRequestRead*>(pRequest);

		if (pReadReq->pBuf == &pLoadReq->hdr)
		{
			if (bytesTransferred != sizeof(pLoadReq->hdr))
			{
				X_ERROR("Model", "Failed to read model header. Got: 0x%" PRIx32 " need: 0x%" PRIxS, bytesTransferred, sizeof(pLoadReq->hdr));
				onLoadRequestFail(pLoadReq);
				return;
			}

			if (!pLoadReq->hdr.isValid())
			{
				X_ERROR("Model", "\"%s\" model header is invalid", pModel->getName().c_str());
				onLoadRequestFail(pLoadReq);
				return;
			}

			// we need to allocate memory and dispatch a read request for it.
			uint32_t datasize = pLoadReq->hdr.dataSize;
			X_ASSERT(datasize > 0, "Datasize must be positive")(datasize);
			pLoadReq->data = core::makeUnique<uint8_t[]>(blockArena_, datasize, 16);
			
			core::IoRequestRead read;
			read.callback.Bind<XModelManager, &XModelManager::IoRequestCallback>(this);
			read.pUserData = pLoadReq;
			read.pFile = pFile;
			read.offset = sizeof(pLoadReq->hdr);
			read.dataSize = datasize;
			read.pBuf = pLoadReq->data.ptr();
			gEnv->pFileSys->AddIoRequestToQue(read);		
		}
		else
		{
			if (bytesTransferred != pLoadReq->hdr.dataSize)
			{
				X_ERROR("Model", "Failed to read model data. Got: 0x%" PRIx32 " need: 0x%" PRIx32, bytesTransferred, pLoadReq->hdr.dataSize);
				onLoadRequestFail(pLoadReq);
				return;
			}

			gEnv->pJobSys->CreateMemberJobAndRun<XModelManager>(this, &XModelManager::ProcessData_job,
				pLoadReq JOB_SYS_SUB_ARG(core::profiler::SubSys::ENGINE3D));
		}
	}
}

void XModelManager::ProcessData_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_UNUSED(jobSys);
	X_UNUSED(threadIdx);
	X_UNUSED(pJob);
	X_UNUSED(pData);

	ModelLoadRequest* pLoadReq = static_cast<ModelLoadRequest*>(X_ASSERT_NOT_NULL(pData));
	XModel* pModel = pLoadReq->pModel;

	
	pModel->processData(pLoadReq->hdr, std::move(pLoadReq->data), X_ASSERT_NOT_NULL(engine::gEngEnv.pMaterialMan_));


	loadRequestCleanup(pLoadReq);
}



void XModelManager::Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name)
{
	X_UNUSED(jobSys);
#if 0
	const char* fileExt;

	fileExt = core::strUtil::FileExtension(name);
	if (fileExt)
	{
		if (core::strUtil::IsEqual(MODEL_FILE_EXTENSION, fileExt))
		{
			// all asset names need forward slashes, for the hash.
			core::Path<char> path(name);
			path.replaceAll('\\', '/');
			path.removeExtension();

			ReloadModel(path.fileName());
			return true;
		}
	}
	return false;
#else
	X_UNUSED(name);
#endif
}

void XModelManager::Cmd_ListModels(core::IConsoleCmdArgs* pCmd)
{
	// optional search criteria
	const char* pSearchPatten = nullptr;

	if (pCmd->GetArgCount() >= 2) {
		pSearchPatten = pCmd->GetArg(1);
	}

	listModels(pSearchPatten);
}

void XModelManager::Cmd_ReloadModel(core::IConsoleCmdArgs* pCmd)
{
	if (pCmd->GetArgCount() < 2) {
		X_WARNING("Model", "reloadModel <name>");
		return;
	}

	const char* pName = pCmd->GetArg(1);

	reloadModel(pName);
}

X_NAMESPACE_END
