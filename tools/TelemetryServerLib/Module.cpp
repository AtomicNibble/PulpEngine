#include "stdafx.h"

#include <IConverterModule.h>
#include <IEngineModule.h>
#include <Extension\XExtensionMacros.h>
#include <ModuleExports.h> // needed for gEnv


namespace
{
    TelemSrvLibArena::AllocationPolicy g_TelemSrvAlloc;

} // namespace

TelemSrvLibArena* g_TelemSrvLibArena = nullptr;

X_LINK_ENGINE_LIB("SqLite")

#ifdef X_LIB

X_FORCE_LINK_FACTORY("XEngineModule_SqLite");

#endif // !X_LIB


class XTelemSrvLib : public IEngineModule
{
    X_ENGINE_INTERFACE_SIMPLE(IEngineModule);

    X_ENGINE_GENERATE_SINGLETONCLASS(XTelemSrvLib, "Engine_TelemetryServerLib",
        0x21b9630e, 0xdad5, 0x46e7, 0x8b, 0x87, 0x82, 0x8d, 0xd5, 0xb0, 0x76, 0xb9);

    virtual const char* GetName(void) X_FINAL
    {
        return "TelemetryServerLib";
    }

    virtual bool Initialize(CoreGlobals& env, const CoreInitParams& initParams) X_FINAL
    {
        X_ASSERT_NOT_NULL(gEnv);
        X_ASSERT_NOT_NULL(gEnv->pArena);

        X_UNUSED(initParams);

        g_TelemSrvLibArena = X_NEW(TelemSrvLibArena, gEnv->pArena, "TelemetryServerLibArena")(&g_TelemSrvAlloc, "TelemetryServerLibArena");

        if (!env.pCore->IntializeLoadedEngineModule(X_ENGINE_OUTPUT_PREFIX "SqLite", "Engine_SqLite")) {
            return false;
        }

        return true;
    }

    virtual bool ShutDown(void) X_FINAL
    {
        X_ASSERT_NOT_NULL(gEnv);
        X_ASSERT_NOT_NULL(gEnv->pArena);
        X_DELETE_AND_NULL(g_TelemSrvLibArena, gEnv->pArena);
        return true;
    }
};

X_ENGINE_REGISTER_CLASS(XTelemSrvLib);

XTelemSrvLib::XTelemSrvLib()
{
}

XTelemSrvLib::~XTelemSrvLib()
{
}
