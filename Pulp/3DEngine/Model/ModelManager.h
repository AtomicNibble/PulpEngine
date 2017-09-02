#pragma once

#ifndef X_MODEL_MANAGER_H_
#define X_MODEL_MANAGER_H_

#include <IModel.h>
#include <IModelManager.h>

#include <Assets\AssertContainer.h>
#include <Util\UniquePointer.h>
#include <Containers\Fifo.h>
#include <Time\TimeVal.h>

X_NAMESPACE_DECLARE(core,
	namespace V2 {
		struct Job;
		class JobSystem;
	}

	struct XFileAsync;
	struct IoRequestBase;
	struct IConsoleCmdArgs;
)

X_NAMESPACE_BEGIN(model)

class RenderModel;

struct ModelLoadRequest
{
	ModelLoadRequest(RenderModel* pModel) :
		pFile(nullptr),
		pModel(pModel)
	{
		core::zero_object(hdr);
	}
	core::XFileAsync* pFile;
	RenderModel* pModel;
	core::UniquePointer<uint8_t[]> data;
	core::TimeVal dispatchTime;
	core::TimeVal loadTime;
	ModelHeader hdr;
};

class XModelManager : 
	public IModelManager,
	public core::IXHotReload
{
	typedef core::AssetContainer<RenderModel, MODEL_MAX_LOADED, core::SingleThreadPolicy> ModelContainer;
	typedef ModelContainer::Resource ModelResource;

	typedef core::Array<ModelLoadRequest*> ModelLoadRequestArr;
	typedef core::Fifo<ModelResource*> ModelQueue;

public:
	XModelManager(core::MemoryArenaBase* arena, core::MemoryArenaBase* blockArena);
	~XModelManager() X_OVERRIDE;

	void registerCmds(void);
	void registerVars(void);

	bool init(void);
	void shutDown(void);

	bool asyncInitFinalize(void);
	void dispatchPendingLoads(void);

	XModel* findModel(const char* pModelName) const X_FINAL;
	XModel* loadModel(const char* pModelName) X_FINAL;
	XModel* getDefaultModel(void) const X_FINAL;

	void releaseModel(XModel* pModel);

	void reloadModel(const char* pName);
	void listModels(const char* pSearchPatten = nullptr) const;

	bool waitForLoad(XModel* pModel) X_FINAL; // returns true if load succeed.

private:
	bool initDefaults(void);
	void freeDanglingMaterials(void);
	void releaseResources(XModel* pModel);


	void queueLoadRequest(ModelResource* pModel);
	void dispatchLoadRequest(ModelLoadRequest* pLoadReq);


	// load / processing
	void onLoadRequestFail(ModelLoadRequest* pLoadReq);
	void loadRequestCleanup(ModelLoadRequest* pLoadReq);

	void IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
		core::XFileAsync* pFile, uint32_t bytesTransferred);

	void ProcessData_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);

private:
	// IXHotReload
	void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_OVERRIDE;
	// ~IXHotReload

private:
	void Cmd_ListModels(core::IConsoleCmdArgs* pCmd);
	void Cmd_ReloadModel(core::IConsoleCmdArgs* pCmd);

private:
	core::MemoryArenaBase* arena_;
	core::MemoryArenaBase* blockArena_; // for the model data buffers

	RenderModel*	pDefaultModel_;
	ModelContainer	models_;

	// loading
	core::CriticalSection loadReqLock_;
	core::ConditionVariable loadCond_;

	ModelQueue requestQueue_;
	ModelLoadRequestArr pendingRequests_;
};

X_NAMESPACE_END

#endif // !X_MODEL_MANAGER_H_