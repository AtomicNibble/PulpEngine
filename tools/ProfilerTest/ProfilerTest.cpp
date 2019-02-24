#include "stdafx.h"
#include "ProfilerTest.h"
#include "EngineApp.h"

#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>
#include <Memory\MemoryTrackingPolicies\ExtendedMemoryTracking.h>
#include <Memory\ThreadPolicies\MultiThreadPolicy.h>
#include <Memory\VirtualMem.h>
#include <Util/UniquePointer.h>

#include <Platform\Console.h>

#include <Time/StopWatch.h>

#include <../Telemetry/TelemetryLib.h>

#if TTELEMETRY_LINK
X_LINK_ENGINE_LIB("TelemetryLib")
#endif // !TTELEMETRY_LINK


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

        core::CriticalSection cs0, cs1;

        core::StopWatch timer;
        int32_t total = 0;

        ttSetThreadName(ctx, ::GetCurrentThreadId(), "BackgroundThread");
        ttSetLockName(ctx, &cs0, "Magic lock");
        ttSetLockName(ctx, &cs1, "Stu's lock");

        for (int i = 0; i < 15; i++)
        {
            ttTick(ctx);

            ttZone(ctx, "Aint no camel like me!");

            // alloc me baby.
            ttAlloc(ctx, (void*)0x12345678, 0x300 * (i + 1))
            ttFree(ctx, (void*)0x12345678)

            for (int x = 0; x < 10; x++)
            {
                {
                    ScopedLockTelemetry lock(cs0);

                    ttZone(ctx, "Slap my goat!");
                    total++;
                    core::Thread::sleep(0);
                }

#if 0
                {
                   ttZoneFilterd(ctx, 2, "Sometimes filtered");
                    core::Thread::sleep(0);
                }
#endif

                for (int j = 0; j < 50; j++)
                {
                    ttTick(ctx);
                    ttZone(ctx, "Meoooow");

                    for (int y = 0; y < 100; y++)
                    {
                        ttEnter(ctx, "Yep")
                        ttLeave(ctx)
                    }

                }
            }

            // Lock me up like a goat.
            ScopedLockTelemetry lock(cs1);
            core::Thread::sleep(0);
        }

        ttFlush(ctx);

        auto ellapsed = timer.GetMilliSeconds();
        X_LOG0("ProfilerTest", "total %i %fms", total, ellapsed);

        return core::Thread::ReturnValue(0);
    }


    void LogFunc(void* pUserData, LogType type, const char* pMsgNullTerm, tt_int32 lenWithoutTerm)
    {
        X_UNUSED(pUserData);
        X_UNUSED(type);
        X_UNUSED(pMsgNullTerm);
        X_UNUSED(lenWithoutTerm);

#if 0
        switch (type)
        {
            case LogType::Msg:
                X_LOG0("Telemetry", pMsgNullTerm);
                break;
            case LogType::Warning:
                X_WARNING("Telemetry", pMsgNullTerm);
                break;
            case LogType::Error:
                X_ERROR("Telemetry", pMsgNullTerm);
                break;
       }
#endif
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
        if (!ttLoadLibary()) {
            return 1;
        }

        // Setup telem.
        if (!ttInit()) {
            return 1;
        }


#if TTELEMETRY_ENABLED

        const size_t telemBufSize = 1024 * 1024 * 2;
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
            EngineApp engine;

            if (engine.Init(hInstance, lpCmdLine)) {
                X_ASSERT_NOT_NULL(gEnv);
                X_ASSERT_NOT_NULL(gEnv->pCore);

                // now engine logging is init redirect logs here.
                ttSetContextLogFunc(ctx, LogFunc, nullptr);

                gEnv->pConsoleWnd->redirectSTD();

                core::Thread thread;
                thread.create("Test");
                thread.start(threadFunc);

                thread.join();
                thread.destroy();

                gEnv->pConsoleWnd->pressToContinue();
            }
        }

        ttShutdownContext(ctx);
    }

    ttShutDown();

    return 0;
}
