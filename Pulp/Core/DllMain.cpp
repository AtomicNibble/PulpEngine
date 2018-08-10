#include "StdAfx.h"

#include <ModuleExports.h>

#include "Core.h"

core::MemoryArenaBase* g_coreArena = nullptr;

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
#endif // !X_LIB

extern "C" {
IPCORE_API ICore* CreateCoreInterface(const CoreInitParams& startupParams)
{
    X_ASSERT_NOT_NULL(startupParams.pCoreArena);

    XCore* pCore = NULL;

    g_coreArena = startupParams.pCoreArena;

    if (!g_coreArena) {
        return nullptr;
    }

    pCore = X_NEW_ALIGNED(XCore, startupParams.pCoreArena, "XCore", 16);

    LinkModule(pCore, "Core");

#if defined(X_LIB)
    if (pCore) {
        IEngineFactoryRegistryImpl* pGoatFactoryImpl = static_cast<IEngineFactoryRegistryImpl*>(pCore->GetFactoryRegistry());
        pGoatFactoryImpl->RegisterFactories(g_pHeadToRegFactories);
    }
#endif

    if (!pCore->Init(startupParams)) {
        // wait till any async events either fail or succeded i don't care.
        // this is just to remove races on shutdown logic.
        (void)pCore->InitAsyncWait();

        if (gEnv && gEnv->pLog) {
            X_ERROR("Core", "Failed to init core");
        }

        X_DELETE(pCore, startupParams.pCoreArena);
        return nullptr;
    }

    return pCore;
}
}
