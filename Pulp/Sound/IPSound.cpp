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
	core::SimpleBoundsChecking,
	core::SimpleMemoryTracking,
	//	core::FullMemoryTracking,
	//	core::ExtendedMemoryTracking,
	core::SimpleMemoryTagging
> SoundArena;

// the allocator dose not check for leaks so it
// dose not need to go out of scope.
namespace {
	core::MallocFreeAllocator g_SoundAlloc;
}


core::MemoryArenaBase* g_SoundArena = nullptr;


//////////////////////////////////////////////////////////////////////////
class XEngineModule_Sound : public IEngineModule
{
	X_POTATO_GENERATE_SINGLETONCLASS(XEngineModule_Sound, "Engine_Sound");
	//////////////////////////////////////////////////////////////////////////
	virtual const char *GetName() X_OVERRIDE{ return "Sound"; };

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize(SCoreGlobals &env, const SCoreInitParams &initParams) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);
		X_ASSERT_NOT_NULL(gEnv->pConsole);
		X_UNUSED(initParams);

		sound::ISound* pSound = nullptr;

		g_SoundArena = X_NEW(SoundArena, gEnv->pArena, "SoundArena")(&g_SoundAlloc, "SoundArena");
		pSound = X_NEW(sound::XSound, g_SoundArena, "SoundSys");

		pSound->RegisterVars();
		pSound->RegisterCmds();

		if (!pSound->Init()) {
			X_DELETE(pSound, g_SoundArena);
			return false;
		}

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



X_POTATO_REGISTER_CLASS(XEngineModule_Sound);

XEngineModule_Sound::XEngineModule_Sound()
{
};

XEngineModule_Sound::~XEngineModule_Sound()
{
};

