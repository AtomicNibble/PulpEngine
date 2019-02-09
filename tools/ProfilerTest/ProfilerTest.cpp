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

    X_INLINE uint64_t getTicks(void)
    {
        return __rdtsc();
    }

    struct BlockInfo
    {
        BlockInfo(const core::SourceInfo& sourceInfo, const char* pLabel) :
            sourceInfo_(sourceInfo),
            pLabel_(pLabel)
        {}

        core::SourceInfo sourceInfo_;
        const char* pLabel_;
    };

    struct BlockData
    {
        uint64_t start;
        uint64_t end;
        uint64_t childTime;
    };

    struct ProfileBlockScope
    {
        ProfileBlockScope(const BlockInfo& info) :
            info_(info)
        {
            data_.start = getTicks();
        }

        ~ProfileBlockScope() 
        {
            data_.end = getTicks();
        }

        const BlockInfo& info_;
        BlockData data_;
    };

    // one per thread :D
    struct ThreadData
    {
        // This is called by the CRR in response to DllMain DLL_THREAD_ATTACH
        ThreadData()
        {
            threadId = core::Thread::getCurrentID();

        }

        uint32_t threadId;
        

    };

    thread_local ThreadData threadData;


#define X_PROFILE_TRACE(pLabel) \
    static const BlockInfo __blockInfo(X_SOURCE_INFO, pLabel); \
    ProfileBlockScope ___scope(__blockInfo);

    void Test0(void)
    {


        core::Thread::sleep(10);

    }

    core::Thread::ReturnValue threadFunc(const core::Thread& thread)
    {
        X_UNUSED(thread);

        {

            core::Thread::sleep(10);
        }

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
        if (!telem::Init()) {
            return 1;
        }

        const size_t telemBufSize = 1024 * 1024;
        auto telemBuf = core::makeUnique<uint8_t[]>(&arena, telemBufSize, 16);

        telem::TraceContexHandle ctx;
        telem::InitializeContext(ctx, telemBuf.ptr(), telemBufSize);

        {
            core::Thread thread;
            thread.create("Test");
            thread.start(threadFunc);

            {
                EngineApp engine;

                if (engine.Init(hInstance, lpCmdLine))
                {
                    X_ASSERT_NOT_NULL(gEnv);
                    X_ASSERT_NOT_NULL(gEnv->pCore);


                    thread.join();
                    thread.destroy();

                    gEnv->pConsoleWnd->pressToContinue();
                }
            }
        }
    }

    telem::ShutDown();

    return 0;
}
