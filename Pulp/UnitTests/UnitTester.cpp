#include "stdafx.h"
#include "UnitTester.h"
#include "EngineApp.h"


#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>
#include <Memory\ThreadPolicies\MultiThreadPolicy.h>

#include <Threading\CriticalSection.h>


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
X_LINK_LIB("engine_ImgLib")


X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_Font@@0V12@A")
X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_Script@@0V12@A")
X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_Render@@0V12@A")
X_FORCE_SYMBOL_LINK("?factory__@XFactory@XConverterLib_Img@@0V12@A")


#endif // !X_LIB

void InitRootDir(void)
{
#ifdef WIN32
	WCHAR szExeFileName[_MAX_PATH] = { 0 };
	GetModuleFileNameW(GetModuleHandleW(NULL), szExeFileName, sizeof(szExeFileName));	

	core::Path<wchar_t> path(szExeFileName);

	path.removeFileName();
	path.removeTrailingSlash();

	if (!SetCurrentDirectoryW(path.c_str())) {
		::MessageBoxW(0, L"Failed to set current directory", L"Error", MB_OK);
		ExitProcess(static_cast<uint32_t>(-1));
	}
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

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	g_hInstance = hInstance;
//	InitRootDir();

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

		//	::testing::GTEST_FLAG(filter) = "Threading.*";
			X_LOG0("TESTS", "Running unit tests...");
			testing::InitGoogleTest(&__argc, __wargv);

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
