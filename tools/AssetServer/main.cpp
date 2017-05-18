#include "stdafx.h"
#include "EngineApp.h"
#include "AssetServer.h"


#define _LAUNCHER
#include <ModuleExports.h>

#ifdef X_LIB

struct XRegFactoryNode* g_pHeadToRegFactories = nullptr;

X_LINK_LIB("engine_Core")
X_LINK_LIB("engine_RenderNull")

// X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_Render@@0V12@A")
// X_FORCE_LINK_FACTORY("XEngineModule_Render")

X_FORCE_SYMBOL_LINK("?s_factory@XEngineModule_Render@render@Potato@@0V?$XSingletonFactory@VXEngineModule_Render@render@Potato@@@@A");


#endif // !X_LIB


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
> AssertServerArena;


X_USING_NAMESPACE;

#if X_PLATFORM_WIN32
namespace
{
	EngineApp* pEngine = nullptr;

	BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
	{
		if (dwCtrlType == CTRL_CLOSE_EVENT || dwCtrlType == CTRL_C_EVENT)
		{
			if (pEngine) {
				pEngine->DestoryIcon();
				pEngine = nullptr;
			}
		}

		return FALSE;
	}
}
#endif // !X_PLATFORM_WIN32

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	core::Console Console(X_WIDEN(X_ENGINE_NAME) L" - AssetServer");
	Console.RedirectSTD();
	Console.SetSize(60, 40, 2000);
	Console.MoveTo(10, 10);

	core::MallocFreeAllocator allocator;
	AssertServerArena arena(&allocator, "AssetServerArena");

	bool res = false;

	{
		EngineApp engine;

		if (engine.Init(hInstance, &arena, lpCmdLine, Console))
		{
#if X_PLATFORM_WIN32
			pEngine = &engine;
			if (!SetConsoleCtrlHandler(HandlerRoutine, TRUE)) {
				return -1;
			}
#endif // !X_PLATFORM_WIN32

			assetServer::AssetServer as(&arena);

			as.Run(false);

			engine.PumpMessages();
		}

#if X_PLATFORM_WIN32
		pEngine = nullptr;
#endif // !X_PLATFORM_WIN32
	}

	return 0;
}

