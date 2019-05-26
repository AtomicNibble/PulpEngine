#include "stdafx.h"
#include "EngineApp.h"

#include <../TelemetryServerLib/IServer.h>

#include <IConsole.h>

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

using namespace core::string_view_literals;

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

        core::ICVar* pLogVerbosity = gEnv->pConsole->getCVar(core::string_view("log_verbosity"));
        pLogVerbosity->Set(0);

        TelemetryServerArena::AllocationPolicy allocator;
        TelemetryServerArena arena(&allocator, "TelemetryServerArena");

        // create the server.
        {
            auto srv = telemetry::createServer(&arena);

            // TODO: i tihnk core should store all args as utf-8
            auto traceFilePath = gEnv->pCore->GetCommandLineArg("trace"_sv);

            if (!traceFilePath.empty())
            {
                core::Path<> path(traceFilePath.begin(), traceFilePath.end());

                if (!srv->ingestTraceFile(path)) {
                    X_ERROR("TelemSrv", "Failed to ingrest trace file: \"%s\"", path.c_str());
                    return 1;
                }
            }
            else
            {

                srv->loadApps();

                if (!srv->listen()) {
                    return 1;
                }

            }
        }
    }

    return 0;
}


