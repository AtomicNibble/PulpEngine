#include "stdafx.h"
#include "EngineApp.h"

#include <Platform\Console.h>


#define _LAUNCHER
#include <ModuleExports.h>


HINSTANCE g_hInstance = 0;

#ifdef X_LIB

struct XRegFactoryNode* g_pHeadToRegFactories = nullptr;

X_LINK_LIB("engine_Core")
X_LINK_LIB("engine_RenderNull")

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
> ServerTestArena;

core::MemoryArenaBase* g_arena = nullptr;


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
 
	g_hInstance = hInstance;

	{

		core::Console Console(X_WIDEN(X_ENGINE_NAME) L" - AssetServer Test Client");
		Console.RedirectSTD();
		Console.SetSize(60, 40, 2000);
		Console.MoveTo(10, 10);

		core::MallocFreeAllocator allocator;
		ServerTestArena arena(&allocator, "AssetServerTestArena");
		g_arena = &arena;

		bool res = false;

		EngineApp engine;

		if (engine.Init(lpCmdLine, Console))
		{
	
			

		}

		Console.PressToContinue();
		engine.ShutDown();
	}

	return 0;
}

