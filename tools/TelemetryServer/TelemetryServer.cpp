#include "stdafx.h"
#include "EngineApp.h"

#include <../TelemetryServerLib/IServer.h>

#define _LAUNCHER
#include <ModuleExports.h>

#ifdef X_LIB

struct XRegFactoryNode* g_pHeadToRegFactories = nullptr;

X_LINK_ENGINE_LIB("Core")
X_FORCE_LINK_FACTORY("XTelemSrvLib");

#endif // !X_LIB

X_LINK_ENGINE_LIB("TelemetryServerLib")


namespace
{
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
    >
        TelemetryServerArena;


} // namespace



int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    X_UNUSED(hPrevInstance);
    X_UNUSED(nCmdShow);

    {
        EngineApp app;

        if (!app.Init(hInstance, lpCmdLine)) {
            return 1;
        }

        TelemetryServerArena::AllocationPolicy allocator;
        TelemetryServerArena arena(&allocator, "TelemetryServerArena");

        // create the server.
        auto srv = telemetry::createServer(&arena);

        srv->loadApps();

        if (!srv->listen()) {
            return 1;
        }

        // TODO: cleanup server?
    }

    return 0;
}


