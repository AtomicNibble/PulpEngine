#include "stdafx.h"
#include "EngineApp.h"

#include <Platform/Console.h>

#include <../TelemetryServerLib/TelemetryServerLib.h>

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
        TelemetryTraceImportArena;


} // namespace

using namespace core::string_view_literals;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    X_UNUSED(hPrevInstance);
    X_UNUSED(nCmdShow);

    int res = 0;

    {
        EngineApp app;

        if (!app.Init(hInstance, lpCmdLine)) {
            return 1;
        }

        TelemetryTraceImportArena::AllocationPolicy allocator;
        TelemetryTraceImportArena arena(&allocator, "TelemetryTraceImportArena");

        {
            auto traceFilePath = gEnv->pCore->GetCommandLineArg("trace"_sv);

            if (!traceFilePath.empty())
            {
                X_ERROR("TraceImport", "Missing trace arg");
                res = 1;
            }
            else
            {
                core::Path<> path(traceFilePath.begin(), traceFilePath.end());

                telemetry::TraceImport ti;

                if (!ti.ingestTraceFile(path)) {
                    X_ERROR("TraceImport", "Failed to ingest trace file: \"%s\"", path.c_str());
                    res = 1;
                }

                gEnv->pConsoleWnd->pressToContinue();
            }
           
        }
    }

    return res;
}


