#include "stdafx.h"
#include "EngineApp.h"

#include "TelemetryViewer.h"

#define _LAUNCHER
#include <ModuleExports.h>

#ifdef X_LIB

struct XRegFactoryNode* g_pHeadToRegFactories = nullptr;

X_LINK_ENGINE_LIB("Core")
X_LINK_ENGINE_LIB("TelemetryServerLib")

X_FORCE_LINK_FACTORY("XTelemSrvLib");

#endif // !X_LIB


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

    TelemetryViewerArena::AllocationPolicy allocator;
    TelemetryViewerArena arena(&allocator, "TelemetryViewerArena");

    g_arena = &arena;

    {
        EngineApp app;

        if (!app.Init(hInstance, &arena, lpCmdLine)) {
            return 1;
        }

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


