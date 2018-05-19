#include "stdafx.h"
#include "Benchmarks.h"
#include "EngineApp.h"

#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>
#include <Memory\MemoryTrackingPolicies\ExtendedMemoryTracking.h>
#include <Memory\ThreadPolicies\MultiThreadPolicy.h>
#include <Memory\AllocationPolicies\PoolAllocator.h>
#include <Memory\AllocationPolicies\GrowingBlockAllocator.h>
#include <Memory\VirtualMem.h>
#include <Memory\SimpleMemoryArena.h>

#include <Platform\Console.h>

#define _LAUNCHER
#include <ModuleExports.h>

// Google Benchmark
X_LINK_LIB("shlwapi")

#if X_DEBUG == 1
X_LINK_LIB("benchmarkd")
#else
X_LINK_LIB("benchmark")
#endif

#if 1
typedef core::SimpleMemoryArena<
    core::GrowingBlockAllocator>
    BenchmarkArena;
#else
typedef core::MemoryArena<
    // core::MallocFreeAllocator,
    core::GrowingBlockAllocator,

    // core::MultiThreadPolicy<core::CriticalSection>,
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
    >
    BenchmarkArena;
#endif

core::MemoryArenaBase* g_arena = nullptr;

int APIENTRY WinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    X_UNUSED(hPrevInstance, nCmdShow);

    core::Console console(L"Engine Benchmark Log");
    console.redirectSTD();
    console.setSize(150, 60, 8000);
    console.moveTo(10, 10);

    BenchmarkArena::AllocationPolicy allocator;
    BenchmarkArena arena(&allocator, "BenchmarkArena");

    g_arena = &arena;

    {
        EngineApp engine;

        if (engine.Init(hInstance, ::GetCommandLineW(), console)) {
            X_ASSERT_NOT_NULL(gEnv);
            X_ASSERT_NOT_NULL(gEnv->pCore);

            ::benchmark::Initialize(&__argc, __argv);
            ::benchmark::RunSpecifiedBenchmarks();

            if (lpCmdLine && !core::strUtil::FindCaseInsensitive(lpCmdLine, "-CI")) {
                console.pressToContinue();
            }
        }
    }

    return 0;
}
