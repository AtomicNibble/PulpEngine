#pragma once


#include <IAnimManager.h>
#include <IAnimation.h>

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


X_NAMESPACE_BEGIN(anim)

class Anim;

struct AnimLoadRequest
{
	AnimLoadRequest(Anim* pAnim) :
		pFile(nullptr),
		pAnim(pAnim)
	{
		// core::zero_object(hdr);
	}
	core::XFileAsync* pFile;
	Anim* pAnim;
	core::UniquePointer<uint8_t[]> data;
	core::TimeVal dispatchTime;
	core::TimeVal loadTime;
	AnimHeader hdr;
};


class AnimManager :
	public IAnimManager,
	public core::IXHotReload
{
	typedef core::AssetContainer<Anim, ANIM_MAX_LOADED, core::SingleThreadPolicy> AnimContainer;
	typedef AnimContainer::Resource AnimResource;

	typedef core::Array<AnimLoadRequest*> AnimLoadRequestArr;
	typedef core::Fifo<AnimResource*> AnimQueue;

public:
	AnimManager(core::MemoryArenaBase* arena, core::MemoryArenaBase* blockArena);
	~AnimManager() X_OVERRIDE;

	void registerCmds(void);
	void registerVars(void);

	bool init(void);
	void shutDown(void);

	bool asyncInitFinalize(void);
	void dispatchPendingLoads(void);

	Anim* findAnim(const char* pAnimName) const X_FINAL;
	Anim* loadAnim(const char* pAnimName) X_FINAL;

	void releaseAnim(Anim* pAnim);

	void reloadAnim(const char* pName);
	void listAnims(const char* pSearchPatten = nullptr) const;

	// returns true if load succeed.
	bool waitForLoad(core::AssetBase* pAnim) X_FINAL; 
	bool waitForLoad(Anim* pAnim) X_FINAL;

private:
	void freeDanglingAnims(void);
	void releaseResources(Anim* pAnim);

	void queueLoadRequest(AnimResource* pAnim);
	void dispatchLoadRequest(AnimLoadRequest* pLoadReq);

	// load / processing
	void onLoadRequestFail(AnimLoadRequest* pLoadReq);
	void loadRequestCleanup(AnimLoadRequest* pLoadReq);

	void IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
		core::XFileAsync* pFile, uint32_t bytesTransferred);

	void ProcessData_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);


private:
	// IXHotReload
	void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_OVERRIDE;
	// ~IXHotReload

private:
	void Cmd_ListAnims(core::IConsoleCmdArgs* pCmd);
	void Cmd_ReloadAnim(core::IConsoleCmdArgs* pCmd);


private:
	core::MemoryArenaBase* arena_;
	core::MemoryArenaBase* blockArena_; // for the anims data buffers

	AnimContainer	anims_;

	// loading
	core::CriticalSection loadReqLock_;
	core::ConditionVariable loadCond_;

	AnimQueue requestQueue_;
	AnimLoadRequestArr pendingRequests_;
};



X_NAMESPACE_END