#include "stdafx.h"
#include "ProfilerTest.h"
#include "EngineApp.h"

#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>
#include <Memory\MemoryTrackingPolicies\ExtendedMemoryTracking.h>
#include <Memory\ThreadPolicies\MultiThreadPolicy.h>
#include <Memory\VirtualMem.h>
#include <Util/UniquePointer.h>

#include <Platform\Console.h>


#include <../Telemetry/TelemetryLib.h>
X_LINK_ENGINE_LIB("TelemetryLib")


#define _LAUNCHER
#include <ModuleExports.h>

#ifdef X_LIB
struct XRegFactoryNode* g_pHeadToRegFactories = nullptr;

X_LINK_ENGINE_LIB("Core")
X_LINK_ENGINE_LIB("RenderNull")

X_FORCE_SYMBOL_LINK("?s_factory@XEngineModule_Render@render@Potato@@0V?$XSingletonFactory@VXEngineModule_Render@render@Potato@@@@A");

#endif // !X_LIB


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
    >
    BenchmarkArena;

core::MemoryArenaBase* g_arena = nullptr;

namespace
{
#if TTELEMETRY_ENABLED
    telem::ContexHandle ctx;
#endif // !TTELEMETRY_ENABLED

    template<class ThreadPolicy>
    class ScopedLockTelemetry
    {
    public:
        inline explicit ScopedLockTelemetry(ThreadPolicy& policy) :
            policy_(policy)
        {
            ttTryLock(ctx, &policy_, "Lock!");

            policy_.Enter();

            ttEndTryLock(ctx, &policy_, TtLockResult::Acquired);
            ttSetLockState(ctx, &policy_, TtLockState::Locked);
        }

        inline ~ScopedLockTelemetry(void) 
        {
            policy_.Leave();
            ttSetLockState(ctx, &policy_, TtLockState::Released);
        }

    private:
        X_NO_COPY(ScopedLockTelemetry);
        X_NO_ASSIGN(ScopedLockTelemetry);

        ThreadPolicy& policy_;
    };


    core::Thread::ReturnValue threadFunc(const core::Thread& thread)
    {
        X_UNUSED(thread);

        core::CriticalSection cs;

        for (int i = 0; i < 15; i++)
        {
            ttTick(ctx);

            ttZone(ctx, "Aint no camel like me!");

            for (int x = 0; x < 500; x++)
            {
                {
                    ttZone(ctx, "Slap my goat!");

                    core::Thread::sleep(0);
                }

                {
                    ttZoneFilterd(ctx, 2, "Sometimes filtered");
                  
                    core::Thread::sleep(0);
                }

                for (int y = 0; y < 10; y++)
                {
                    ttEnter(ctx, "Yep")
                    ttLeave(ctx)
                }
            }

            // Lock me up like a goat.
            ScopedLockTelemetry lock(cs);
            core::Thread::sleep(0);
        }

        ttFlush(ctx);

        return core::Thread::ReturnValue(0);
    }


} // namespace


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    X_UNUSED(hPrevInstance, nCmdShow);

    BenchmarkArena::AllocationPolicy allocator;
    BenchmarkArena arena(&allocator, "ProfileTestArena");

    g_arena = &arena;

    {
        // Setup telem.
        if (!ttInit()) {
            return 1;
        }

#if TTELEMETRY_ENABLED

        const size_t telemBufSize = 1024 * 1024;
        auto telemBuf = core::makeUnique<uint8_t[]>(&arena, telemBufSize, 64);

        ttInitializeContext(ctx, telemBuf.ptr(), telemBufSize);

        auto res = ttOpen(ctx, 
            X_ENGINE_NAME " - Engine", 
            X_BUILD_STRING " Version: " X_ENGINE_VERSION_STR " Rev: " X_ENGINE_BUILD_REF_STR,
            "127.0.0.1",
            8001,
            telem::ConnectionType::Tcp,
            1000);

        if (res != telem::Error::Ok) {
            // rip
            return -1;
        }

#endif // TTELEMETRY_ENABLED

        {
            {
                EngineApp engine;

                if (engine.Init(hInstance, lpCmdLine))
                {
                    X_ASSERT_NOT_NULL(gEnv);
                    X_ASSERT_NOT_NULL(gEnv->pCore);

                    gEnv->pConsoleWnd->redirectSTD();

                    core::Thread thread;
                    thread.create("Test");
                    thread.start(threadFunc);


                    thread.join();
                    thread.destroy();

                    gEnv->pConsoleWnd->pressToContinue();
                }
            }
        }
    }

    ttShutDown();

    return 0;
}
