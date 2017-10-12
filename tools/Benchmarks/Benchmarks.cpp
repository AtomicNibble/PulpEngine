#include "stdafx.h"
#include "Benchmarks.h"


#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>
#include <Memory\ThreadPolicies\MultiThreadPolicy.h>

#include <Platform\Console.h>

#define _LAUNCHER
#include <ModuleExports.h>


typedef core::MemoryArena<
	core::MallocFreeAllocator,
	core::MultiThreadPolicy<core::CriticalSection>,
#if X_ENABLE_MEMORY_DEBUG_POLICIES
	core::SimpleBoundsChecking,
	core::SimpleMemoryTracking,
	core::SimpleMemoryTagging
#else
	core::NoBoundsChecking,
	core::NoMemoryTracking,
	core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
> BenchmarkArena;



int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	core::MemoryArenaBase* g_arena = nullptr;

	core::Console Console(L"Engine Benchmark Log");
	Console.RedirectSTD();
	Console.SetSize(150, 60, 8000);
	Console.MoveTo(10, 10);

	core::MallocFreeAllocator allocator;
	BenchmarkArena arena(&allocator, "BenchmarkArena");

	g_arena = &arena;



	return 0;
}

