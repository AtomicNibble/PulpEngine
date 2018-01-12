#include "stdafx.h"
#include <ModuleExports.h>

#include <ICore.h>
#include <IEngineModule.h>

#include "BaseInput.h"
#include "win32/Win32Input.h"

#include <Extension\XExtensionMacros.h>

#include <Memory\MemoryTrackingPolicies\FullMemoryTracking.h>

X_USING_NAMESPACE;


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

> InputArena;


// the allocator dose not check for leaks so it
// dose not need to go out of scope.
namespace {
	core::MallocFreeAllocator g_InputAlloc;
}

core::MemoryArenaBase* g_InputArena = nullptr;



//////////////////////////////////////////////////////////////////////////
class XEngineModule_Input : public IEngineModule
{
	X_ENGINE_INTERFACE_SIMPLE(IEngineModule);

	X_ENGINE_GENERATE_SINGLETONCLASS(XEngineModule_Input, "Engine_Input",	
	 0x334db037, 0xc459, 0x4b59, 0x9b, 0x3c, 0x3f, 0x59, 0x79, 0x2, 0x8c, 0x8d);


	//////////////////////////////////////////////////////////////////////////
	virtual const char* GetName(void) X_OVERRIDE{ return "Input"; };

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize(SCoreGlobals &env, const SCoreInitParams &initParams) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);
//		X_ASSERT_NOT_NULL(gEnv->pMalloc);

		ICore* pCore = env.pCore;
		input::IInput* pInput = 0;

		LinkModule(pCore, "Input");

		g_InputArena = X_NEW(InputArena, gEnv->pArena, "InputArena")(&g_InputAlloc, "InputArena");

		if (!gEnv->IsDedicated())
		{
			pInput = X_NEW(input::XWinInput, g_InputArena, "Win32Input")(g_InputArena, static_cast<PLATFORM_HWND>(initParams.hWnd));
		}
		else
		{
			pInput = X_NEW(input::XBaseInput, g_InputArena, "XBaseInput")(g_InputArena);
		}		

		env.pInput = pInput;
		return true;
	}

	virtual bool ShutDown(void) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);

		X_DELETE_AND_NULL(g_InputArena, gEnv->pArena);


		return true;
	}
};



X_POTATO_REGISTER_CLASS(XEngineModule_Input);

XEngineModule_Input::XEngineModule_Input()
{
};

XEngineModule_Input::~XEngineModule_Input()
{
};

