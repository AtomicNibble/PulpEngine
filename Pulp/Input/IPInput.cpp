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
	core::SimpleBoundsChecking,
	 core::SimpleMemoryTracking,
	//	core::FullMemoryTracking,
	//	core::ExtendedMemoryTracking,
	core::SimpleMemoryTagging
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
	X_POTATO_GENERATE_SINGLETONCLASS(XEngineModule_Input, "Engine_Input");
	//////////////////////////////////////////////////////////////////////////
	virtual const char *GetName() X_OVERRIDE{ return "Input"; };

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize(SCoreGlobals &env, const SCoreInitParams &initParams) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);
//		X_ASSERT_NOT_NULL(gEnv->pMalloc);

		ICore* pCore = env.pCore;
		input::IInput *pInput = 0;


		g_InputArena = X_NEW_ALIGNED(InputArena, gEnv->pArena, "InputArena", 8)(&g_InputAlloc, "InputArena");


		if (!gEnv->IsDedicated())
		{
			pInput = X_NEW_ALIGNED(input::XWinInput, g_InputArena, "Win32Input", 8)(pCore, (HWND)initParams.hWnd);
		}
		else
		{
			pInput = X_NEW_ALIGNED(input::XBaseInput, g_InputArena, "XBaseInput", 8);
		}		

		if (!pInput->Init())
		{
			pInput->ShutDown();
			X_DELETE(pInput, g_InputArena);
			return false;
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

