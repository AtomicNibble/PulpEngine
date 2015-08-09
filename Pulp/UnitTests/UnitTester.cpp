#include "stdafx.h"
#include "UnitTester.h"
#include "EngineApp.h"
#include "gtest\gtest.h"

#include <Shlwapi.h>

#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>
#include <Memory\ThreadPolicies\MultiThreadPolicy.h>

#include <Threading\CriticalSection.h>

// Google Test
#if X_DEBUG == 1
X_LINK_LIB("gtestd"X_CPUSTRING)
#else
X_LINK_LIB("gtest"X_CPUSTRING)
#endif

X_LINK_LIB("Shlwapi") // GetModuleFileNameW

#define _LAUNCHER
#include <ModuleExports.h>

HINSTANCE g_hInstance = 0;

typedef core::MemoryArena<
	core::MallocFreeAllocator,
	core::MultiThreadPolicy<core::CriticalSection>,
	core::SimpleBoundsChecking,
//	core::SimpleMemoryTracking,
	core::NoMemoryTracking,			// allow leaks in the tests.
	core::SimpleMemoryTagging
> UnitTestArena;


#ifdef X_LIB
struct XRegFactoryNode* g_pHeadToRegFactories = 0;

X_LINK_LIB("engine_Font")
X_LINK_LIB("engine_Core")
X_LINK_LIB("engine_Script")
X_LINK_LIB("engine_3DEngine")
X_LINK_LIB("engine_RenderNull")


X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_Font@@0V12@A")
X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_Script@@0V12@A")
X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_Render@@0V12@A")


#endif // !X_LIB

void InitRootDir()
{
#ifdef WIN32
	WCHAR szExeFileName[_MAX_PATH];
	GetModuleFileNameW(GetModuleHandle(NULL), szExeFileName, sizeof(szExeFileName));	
	PathRemoveFileSpecW(szExeFileName);
	SetCurrentDirectoryW(szExeFileName);
#endif
}

#include <Platform\Console.h>

core::UtAssetCheckerHandler g_AssetChecker;
core::MemoryArenaBase* g_arena = nullptr;

const char* googleTestResTostr(int nRes)
{
	if (nRes == 0)
		return "SUCCESS";
	return "ERROR";
}


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	g_hInstance = hInstance;
//	InitRootDir();

	int nRes = 0;
	EngineApp engine;

	core::Console Console(L"Engine Uint Test Log");
	Console.RedirectSTD();
	Console.SetSize(150, 60, 8000);
	Console.MoveTo(10, 10);

	core::MallocFreeAllocator allocator;
	UnitTestArena arena(&allocator,"UintTestArena");

	g_arena = &arena;

	if (engine.Init(lpCmdLine, Console))
	{		
		gEnv->pCore->RegisterAssertHandler(&g_AssetChecker);

		X_LOG0("TESTS", "Running unit tests...");
		testing::InitGoogleTest(&__argc, __argv);
		nRes = RUN_ALL_TESTS();

		X_LOG0("TESTS", "Tests Complete result: %s", googleTestResTostr(nRes));


		gEnv->pCore->UnRegisterAssertHandler(&g_AssetChecker);

		system("PAUSE");
	}
	return nRes;
}
