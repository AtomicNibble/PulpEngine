#include "stdafx.h"
#include "Converter.h"
#include "EngineApp.h"

#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>
#include <Memory\ThreadPolicies\MultiThreadPolicy.h>

#include <Platform\Console.h>

#define _LAUNCHER
#include <ModuleExports.h>

HINSTANCE g_hInstance = 0;

#ifdef X_LIB

struct XRegFactoryNode* g_pHeadToRegFactories = 0;

X_LINK_LIB("engine_Core")

#endif // !X_LIB


typedef core::MemoryArena<
	core::MallocFreeAllocator,
	core::SingleThreadPolicy,
	core::SimpleBoundsChecking,
	core::NoMemoryTracking,			
	core::SimpleMemoryTagging
> ConverterArena;


core::MemoryArenaBase* g_arena = nullptr;


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	g_hInstance = hInstance;

	EngineApp engine;

	core::Console Console(L"Potato - Converter");
	Console.RedirectSTD();
	Console.SetSize(90, 60, 2000);
	Console.MoveTo(10, 10);


	core::MallocFreeAllocator allocator;
	ConverterArena arena(&allocator, "ConverterArena");
	g_arena = &arena;


	if (engine.Init(lpCmdLine, Console))
	{
		{


		}
		system("PAUSE");
	}

    return 0;
}

