#include "stdafx.h"
#include "EngineApp.h"
#include "AssetServer.h"


#define _LAUNCHER
#include <ModuleExports.h>


HINSTANCE g_hInstance = 0;

#ifdef X_LIB

struct XRegFactoryNode* g_pHeadToRegFactories = 0;

X_LINK_LIB("engine_Core")
X_LINK_LIB("engine_RenderNull")

X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_Render@@0V12@A")

#endif // !X_LIB


typedef core::MemoryArena<
	core::MallocFreeAllocator,
	core::SingleThreadPolicy,
	core::SimpleBoundsChecking,
	core::NoMemoryTracking,
	core::SimpleMemoryTagging
> AssertServerArena;

core::MemoryArenaBase* g_arena = nullptr;

X_USING_NAMESPACE;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	g_hInstance = hInstance;


	core::Console Console(L"Potato - AssetServer");
	Console.RedirectSTD();
	Console.SetSize(60, 40, 2000);
	Console.MoveTo(10, 10);

	core::MallocFreeAllocator allocator;
	AssertServerArena arena(&allocator, "AssetServerArena");
	g_arena = &arena;

	bool res = false;

	{
		EngineApp engine;

		if (engine.Init(lpCmdLine, Console))
		{
			assetServer::AssetServer as;

			as.Run(false);

			engine.PumpMessages();
		}
	}

	return 0;
}

