#include "StdAfx.h"
#include "Core.h"

#include <Platform\Console.h>
#include <ModuleExports.h>

#if !defined(X_LIB)
BOOL APIENTRY DllMain(HANDLE hModule,
    DWORD ul_reason_for_call,
    LPVOID lpReserved)
{
    X_UNUSED(hModule);
    X_UNUSED(lpReserved);

    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}
#endif // X_LIB

extern "C" IPCORE_API ICore* CreateCoreInterface(CoreInitParams& startupParams)
{
    auto* pCore = XCore::CreateInstance();
    if (!pCore) {
        return nullptr;
    }

    LinkModule(pCore, "Core");

#if defined(X_LIB)
    if (pCore) {
        IEngineFactoryRegistryImpl* pGoatFactoryImpl = static_cast<IEngineFactoryRegistryImpl*>(pCore->GetFactoryRegistry());
        pGoatFactoryImpl->RegisterFactories(g_pHeadToRegFactories);
    }
#endif

    if (!pCore->Init(startupParams)) {
        // wait till any async events either fail or succeeded i don't care.
        // this is just to remove races on shutdown logic.
        (void)pCore->InitAsyncWait();

        if (gEnv && gEnv->pLog) {
            X_ERROR("Core", "Failed to init core");

            if (gEnv->pConsoleWnd != nullptr) {
                gEnv->pConsoleWnd->pressToContinue();
            }
        }

        pCore->Release();
        return nullptr;
    }

    return pCore;
}
