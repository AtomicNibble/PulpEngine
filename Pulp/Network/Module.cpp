#include "stdafx.h"
#include <ModuleExports.h>

#include <ICore.h>
#include <IEngineModule.h>

#include <Extension\XExtensionMacros.h>

#include "XNet.h"

X_USING_NAMESPACE;

NetworkArena* g_NetworkArena = nullptr;

namespace
{
    core::MallocFreeAllocator g_NetworkAlloc;
}

//////////////////////////////////////////////////////////////////////////
class XEngineModule_Network : public IEngineModule
{
    X_ENGINE_INTERFACE_SIMPLE(IEngineModule);

    X_ENGINE_GENERATE_SINGLETONCLASS(XEngineModule_Network, "Engine_Network",
        0x8472b6c5, 0xa5e, 0x4463, 0xbd, 0x7f, 0x8f, 0x6c, 0x85, 0xe8, 0xab, 0x7f);

    //////////////////////////////////////////////////////////////////////////
    virtual const char* GetName(void) X_OVERRIDE
    {
        return "Network";
    };

    //////////////////////////////////////////////////////////////////////////
    virtual bool Initialize(SCoreGlobals& env, const SCoreInitParams& initParams) X_OVERRIDE
    {
        X_UNUSED(initParams);
        ICore* pCore = env.pCore;

        LinkModule(pCore, "Network");

        X_ASSERT_NOT_NULL(gEnv);
        X_ASSERT_NOT_NULL(gEnv->pArena);

        // kinky shit.
        g_NetworkArena = X_NEW(NetworkArena, gEnv->pArena, "NetworkArena")(&g_NetworkAlloc, "NetworkArena");
        auto* pNet = X_NEW(net::XNet, g_NetworkArena, "XNet")(g_NetworkArena);

        env.pNet = pNet;
        return true;
    }

    virtual bool ShutDown(void) X_OVERRIDE
    {
        X_ASSERT_NOT_NULL(gEnv);
        X_ASSERT_NOT_NULL(gEnv->pArena);

        X_DELETE_AND_NULL(g_NetworkArena, gEnv->pArena);
        return true;
    }
};

X_ENGINE_REGISTER_CLASS(XEngineModule_Network);

XEngineModule_Network::XEngineModule_Network(){};

XEngineModule_Network::~XEngineModule_Network(){

};
