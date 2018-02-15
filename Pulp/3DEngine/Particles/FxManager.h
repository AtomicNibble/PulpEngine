#pragma once

#include <IEffect.h>

#include <Assets\AssertContainer.h>


X_NAMESPACE_BEGIN(engine)

namespace fx
{

	class Effect;

	class EffectManager :
		public IEffectManager,
		public core::IXHotReload,
		private core::IAssetLoadSink
	{
		typedef core::AssetContainer<Effect, EFFECT_MAX_LOADED, core::SingleThreadPolicy> EffectContainer;
		typedef EffectContainer::Resource EffectResource;

	public:
		EffectManager(core::MemoryArenaBase* arena, core::MemoryArenaBase* blockArena);
		~EffectManager();

		void registerCmds(void);
		void registerVars(void);

		bool init(void);
		void shutDown(void);


		Effect* findEffect(const char* pEffectName) const X_FINAL;
		Effect* loadEffect(const char* pEffectName) X_FINAL;

		void releaseEffect(Effect* pEffect);

		void reloadEffect(const char* pName);
		void listEffects(const char* pSearchPatten = nullptr) const;

		// returns true if load succeed.
		bool waitForLoad(core::AssetBase* pEffect) X_FINAL;
		bool waitForLoad(Effect* pEffect) X_FINAL;

	private:
		void freeDangling(void);
		void releaseResources(Effect* pEffect);

		void addLoadRequest(EffectResource* pEffect);
		void onLoadRequestFail(core::AssetBase* pAsset) X_FINAL;
		bool processData(core::AssetBase* pAsset, core::UniquePointer<char[]> data, uint32_t dataSize) X_FINAL;

	private:
		void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_FINAL;

	private:
		core::MemoryArenaBase* arena_;
		core::MemoryArenaBase* blockArena_; // for the anims data buffers

		core::AssetLoader* pAssetLoader_;

		EffectContainer	effects_;
	};

} // namespace fx

X_NAMESPACE_END