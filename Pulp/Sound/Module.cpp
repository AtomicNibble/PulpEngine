#include "stdafx.h"
#include "IPSound.h"

#include <ModuleExports.h>
#include <IEngineModule.h>

#include "XSound.h"

#include <Extension\XExtensionMacros.h>

X_USING_NAMESPACE;

#include <Memory\MemoryTrackingPolicies\FullMemoryTracking.h>
#include <Memory\MemoryTrackingPolicies\ExtendedMemoryTracking.h>

typedef core::MemoryArena<
    core::MallocFreeAllocator,
    core::SingleThreadPolicy,
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
    SoundArena;

// the allocator dose not check for leaks so it
// dose not need to go out of scope.
namespace
{
    core::MallocFreeAllocator g_SoundAlloc;
}

core::MemoryArenaBase* g_SoundArena = nullptr;

//////////////////////////////////////////////////////////////////////////
class XEngineModule_Sound : public IEngineModule
{
    X_ENGINE_INTERFACE_SIMPLE(IEngineModule);

    X_ENGINE_GENERATE_SINGLETONCLASS(XEngineModule_Sound, "Engine_Sound",
        0x527ab038, 0xbe01, 0x4a98, 0x84, 0x32, 0x9c, 0x9c, 0x26, 0xe3, 0xcc, 0xb3);

    //////////////////////////////////////////////////////////////////////////
    virtual const char* GetName(void) X_OVERRIDE
    {
        return "Sound";
    };

    //////////////////////////////////////////////////////////////////////////
    virtual bool Initialize(SCoreGlobals& env, const CoreInitParams& initParams) X_OVERRIDE
    {
        X_ASSERT_NOT_NULL(gEnv);
        X_ASSERT_NOT_NULL(gEnv->pArena);
        X_ASSERT_NOT_NULL(gEnv->pConsole);
        X_UNUSED(initParams);

        sound::ISound* pSound = nullptr;

        g_SoundArena = X_NEW(SoundArena, gEnv->pArena, "SoundArena")(&g_SoundAlloc, "SoundArena");
        pSound = X_NEW(sound::XSound, g_SoundArena, "SoundSys")(g_SoundArena);

        env.pSound = pSound;
        return true;
    }

    virtual bool ShutDown(void) X_OVERRIDE
    {
        X_ASSERT_NOT_NULL(gEnv);
        X_ASSERT_NOT_NULL(gEnv->pArena);

        X_DELETE_AND_NULL(g_SoundArena, gEnv->pArena);

        return true;
    }
};

X_ENGINE_REGISTER_CLASS(XEngineModule_Sound);

XEngineModule_Sound::XEngineModule_Sound(){};

XEngineModule_Sound::~XEngineModule_Sound(){};
