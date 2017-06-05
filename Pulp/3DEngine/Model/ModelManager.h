#pragma once

#ifndef X_MODEL_MANAGER_H_
#define X_MODEL_MANAGER_H_

#include <IModel.h>

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

class XModel;


struct ModelLoadRequest
{
	ModelLoadRequest(XModel* pModel) :
		pFile(nullptr),
		pModel(pModel)
	{
		core::zero_object(hdr);
	}
	core::XFileAsync* pFile;
	XModel* pModel;
	core::UniquePointer<uint8_t[]> data;
	core::TimeVal dispatchTime;
	core::TimeVal loadTime;
	ModelHeader hdr;
};

class XModelManager :
	public core::IXHotReload
{
	typedef core::AssetContainer<XModel, MODEL_MAX_LOADED, core::SingleThreadPolicy> ModelContainer;
	typedef ModelContainer::Resource ModelResource;

	// we want to be able to remove at random index, and grow without memory address changing.
	// so linked list? or array of pointers?
	// think a intrusive linked list work well here.

	typedef core::Array<ModelLoadRequest*> ModelLoadRequestArr;
	typedef core::Fifo<ModelResource*> ModelQueue;

public:
	XModelManager(core::MemoryArenaBase* arena, core::MemoryArenaBase* blockArena);
	~XModelManager();

	void registerCmds(void);
	void registerVars(void);

	bool init(void);
	void shutDown(void);

	bool asyncInitFinalize(void);

	XModel* findModel(const char* pModelName) const;
	XModel* loadModel(const char* pModelName);
	XModel* getDefaultModel(void);

	void releaseModel(XModel* pModel);

	void reloadModel(const char* pName);
	void listModels(const char* pSearchPatten = nullptr) const;

private:
	bool initDefaults(void);
	void freeDanglingMaterials(void);
	void releaseResources(XModel* pModel);


	void queueLoadRequest(ModelResource* pModel);
	void dispatchPendingLoads(void);
	bool waitForLoad(XModel* pModel); // returns true if load succeed.
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

	XModel*	pDefaultModel_;
	ModelContainer	models_;

	core::CriticalSection loadReqLock_;
	core::ConditionVariable loadCond_;


	ModelQueue requestQueue_;
	ModelLoadRequestArr pendingRequests_;
};

X_NAMESPACE_END

#endif // !X_MODEL_MANAGER_H_