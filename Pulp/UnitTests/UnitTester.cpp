#include "stdafx.h"
#include "UnitTester.h"
#include "EngineApp.h"


#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>
#include <Memory\ThreadPolicies\MultiThreadPolicy.h>

#include <Threading\CriticalSection.h>
#include <Util\Process.h>
#include <Platform\MessageBox.h>

// Google Test
#if X_DEBUG == 1
X_LINK_LIB("gtestd")
#else
X_LINK_LIB("gtest")
#endif

// X_LINK_LIB("Shlwapi") // GetModuleFileNameW

#define _LAUNCHER
#include <ModuleExports.h>

HINSTANCE g_hInstance = 0;

typedef core::MemoryArena<
	core::MallocFreeAllocator,
	core::MultiThreadPolicy<core::CriticalSection>,
#if X_ENABLE_MEMORY_DEBUG_POLICIES
	core::SimpleBoundsChecking,
	core::NoMemoryTracking,
	core::SimpleMemoryTagging
#else
	core::NoBoundsChecking,
	core::NoMemoryTracking,
	core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
> UnitTestArena;


#ifdef X_LIB
struct XRegFactoryNode* g_pHeadToRegFactories = nullptr;

X_LINK_LIB("engine_Font")
X_LINK_LIB("engine_Core")
X_LINK_LIB("engine_Script")
//X_LINK_LIB("engine_3DEngine")
X_LINK_LIB("engine_RenderNull")
X_LINK_LIB("engine_ImgLib")


X_FORCE_LINK_FACTORY("XEngineModule_Font");
X_FORCE_LINK_FACTORY("XEngineModule_Script");
//X_FORCE_LINK_FACTORY("XEngineModule_Render");
X_FORCE_SYMBOL_LINK("?s_factory@XEngineModule_Render@render@Potato@@0V?$XSingletonFactory@VXEngineModule_Render@render@Potato@@@@A");
X_FORCE_LINK_FACTORY("XConverterLib_Img");


#endif // !X_LIB


#include <Platform\Console.h>

core::UtAssetCheckerHandler g_AssetChecker;
core::MemoryArenaBase* g_arena = nullptr;

const char* googleTestResTostr(int nRes)
{
	if (nRes == 0) 
		return "SUCCESS";
	return "ERROR";
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	g_hInstance = hInstance;
	int nRes = -1; // if we never run the tests that is also a fail.

	core::Console Console(L"Engine Uint Test Log");
	Console.RedirectSTD();
	Console.SetSize(150, 60, 8000);
	Console.MoveTo(10, 10);

	core::MallocFreeAllocator allocator;
	UnitTestArena arena(&allocator,"UintTestArena");

	g_arena = &arena;

	EngineApp engine;

	if (engine.Init(lpCmdLine, Console))
	{		
		{
			X_ASSERT_NOT_NULL(gEnv);
			X_ASSERT_NOT_NULL(gEnv->pCore);
			gEnv->pCore->RegisterAssertHandler(&g_AssetChecker);

			::testing::GTEST_FLAG(filter) = "-*Fiber*";
			//::testing::GTEST_FLAG(filter) = "*JobSystem2Empty_parallel_data:-*Fiber*";
			//::testing::GTEST_FLAG(filter) = "*ECS*";
			X_LOG0("TESTS", "Running unit tests...");
			testing::InitGoogleTest(&__argc, __wargv);

			core::Process pro = core::Process::GetCurrent();
			pro.SetPriorityClass(core::Process::Priority::REALTIME);

			nRes = RUN_ALL_TESTS();

			X_LOG0("TESTS", "Tests Complete result: %s", googleTestResTostr(nRes));


			gEnv->pCore->UnRegisterAssertHandler(&g_AssetChecker);
		}

		if (lpCmdLine && !core::strUtil::FindCaseInsensitive(lpCmdLine, L"-CI")) {
			system("PAUSE");
		}
	}
	return nRes;
}
