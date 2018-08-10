#pragma once

#include <IEffect.h>

#include <Assets\AssertContainer.h>
#include <Memory\AllocationPolicies\PoolAllocator.h>

#include "Vars\EffectVars.h"

X_NAMESPACE_DECLARE(core,
                    struct IConsoleCmdArgs;)

X_NAMESPACE_BEGIN(engine)

namespace fx
{
    class Effect;
    class Emitter;

    class EffectManager : public IEffectManager
        , private core::IAssetLoadSink
    {
        typedef core::AssetContainer<Effect, EFFECT_MAX_LOADED, core::SingleThreadPolicy> EffectContainer;
        typedef EffectContainer::Resource EffectResource;
        typedef EffectContainer AssetContainer;

        typedef core::MemoryArena<
            core::PoolAllocator,
            core::MultiThreadPolicy<core::Spinlock>,
#if X_ENABLE_MEMORY_DEBUG_POLICIES
            core::SimpleBoundsChecking,
            core::SimpleMemoryTracking,
            core::SimpleMemoryTagging
#else
            core::NoBoundsChecking,
            core::NoMemoryTracking,
            core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
            >
            PoolArena;

        typedef core::ArrayGrowMultiply<Emitter*> EmitterPtrArr;

    public:
        EffectManager(core::MemoryArenaBase* arena, core::MemoryArenaBase* blockArena);
        ~EffectManager();

        void registerCmds(void);
        void registerVars(void);

        bool init(void);
        void shutDown(void);

        Emitter* allocEmmiter(void);
        void freeEmmiter(Emitter* pEmitter);

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
        bool onFileChanged(const core::AssetName& assetName, const core::string& name) X_FINAL;

    private:
        void listAssets(const char* pSearchPattern);

    private:
        void Cmd_ListAssets(core::IConsoleCmdArgs* pCmd);

    private:
        core::MemoryArenaBase* arena_;
        core::MemoryArenaBase* blockArena_; // for the anims data buffers

        core::HeapArea poolHeap_;
        PoolArena::AllocationPolicy poolAllocator_;
        PoolArena poolArena_;

        core::AssetLoader* pAssetLoader_;

        core::CriticalSection cs_;

        EffectContainer effects_;
        EmitterPtrArr emmiters_;
        EffectVars vars_;
    };

} // namespace fx

X_NAMESPACE_END