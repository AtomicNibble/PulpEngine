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
	public core::IXHotReload,
	private core::IAssetLoadSink
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

	Anim* findAnim(const char* pAnimName) const X_FINAL;
	Anim* loadAnim(const char* pAnimName) X_FINAL;

	void releaseAnim(Anim* pAnim);

	void reloadAnim(const char* pName);
	void listAnims(const char* pSearchPatten = nullptr) const;

	// returns true if load succeed.
	bool waitForLoad(core::AssetBase* pAnim) X_FINAL; 
	bool waitForLoad(Anim* pAnim) X_FINAL;

private:
	void freeDangling(void);
	void releaseResources(Anim* pAnim);

	void addLoadRequest(AnimResource* pAnim);
	void onLoadRequestFail(core::AssetBase* pAsset) X_FINAL;
	bool processData(core::AssetBase* pAsset, core::UniquePointer<char[]> data, uint32_t dataSize) X_FINAL;


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

	core::AssetLoader* pAssetLoader_;

	AnimContainer	anims_;

};



X_NAMESPACE_END