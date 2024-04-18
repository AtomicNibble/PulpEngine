#include "stdafx.h"
#include "EngineApp.h"

#include "TelemetryViewer.h"

#include <IConsole.h>

#define _LAUNCHER
#include <ModuleExports.h>

#ifdef X_LIB

struct XRegFactoryNode* g_pHeadToRegFactories = nullptr;

X_LINK_ENGINE_LIB("Core")
X_LINK_ENGINE_LIB("TelemetryServerLib")
X_LINK_ENGINE_LIB("TelemetrySymbols")

X_FORCE_LINK_FACTORY("XTelemSrvLib");
X_FORCE_LINK_FACTORY("XTelemSymLib");

#endif // X_LIB


X_DISABLE_WARNING(4505) // unreferenced local function has been removed

namespace
{
    bool winSockInit(void)
    {
        platform::WSADATA winsockInfo;

        if (platform::WSAStartup(MAKEWORD(2, 2), &winsockInfo) != 0) {
            return false;
        }

        return true;
    }

    void winSockShutDown(void)
    {
        if (platform::WSACleanup() != 0) {
            // rip
            return;
        }
    }

} // namespace

TelemetryViewerArena* g_arena = nullptr;

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

        TelemetryViewerArena::AllocationPolicy allocator;
        TelemetryViewerArena arena(&allocator, "TelemetryViewerArena");

        g_arena = &arena;

        if (!winSockInit()) {
            return 1;
        }

        // TEMP: connect to server on start up.
        // Server srv(g_arena);
        // connectToServer(srv);
        // getAppList(srv);
        {
            telemetry::Client client(&arena);

            if (!telemetry::run(client)) {
                return 1;
            }
        }

        winSockShutDown();
    }

    return 0;
}


