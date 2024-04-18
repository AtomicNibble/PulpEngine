#include "stdafx.h"

#include <IEngineModule.h>
#include <Extension\XExtensionMacros.h>
#include <ModuleExports.h> // needed for gEnv


namespace
{
    TelemSymLibArena::AllocationPolicy g_TelemSymAlloc;

} // namespace

TelemSymLibArena* g_TelemSymLibArena = nullptr;


class XTelemSymLib : public IEngineModule
{
    X_ENGINE_INTERFACE_SIMPLE(IEngineModule);

    X_ENGINE_GENERATE_SINGLETONCLASS(XTelemSymLib, "Engine_TelemetrySymLib",
        0xc2d257a8, 0xfeb7, 0x4815, 0x92, 0x9b, 0x8a, 0x36, 0x30, 0xd2, 0xb4, 0x3e);


    virtual const char* GetName(void) X_FINAL
    {
        return "TelemetrySymLib";
    }

    virtual bool Initialize(CoreGlobals& env, const CoreInitParams& initParams) X_FINAL
    {
        X_ASSERT_NOT_NULL(gEnv);
        X_ASSERT_NOT_NULL(gEnv->pArena);

        X_UNUSED(initParams, env);

        g_TelemSymLibArena = X_NEW(TelemSymLibArena, gEnv->pArena, "TelemetryServerLibArena")(&g_TelemSymAlloc, "TelemetrySymLibArena");

        return true;
    }

    virtual bool ShutDown(void) X_FINAL
    {
        X_ASSERT_NOT_NULL(gEnv);
        X_ASSERT_NOT_NULL(gEnv->pArena);
        X_DELETE_AND_NULL(g_TelemSymLibArena, gEnv->pArena);
        return true;
    }
};

X_ENGINE_REGISTER_CLASS(XTelemSymLib);

XTelemSymLib::XTelemSymLib()
{
}

XTelemSymLib::~XTelemSymLib()
{
}
