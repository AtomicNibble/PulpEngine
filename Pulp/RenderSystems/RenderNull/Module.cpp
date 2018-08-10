#include "stdafx.h"

#define X_NO_DEBUG_HANDLERS
#include <ModuleExports.h>

#include <ICore.h>
#include <IRender.h>
#include <IEngineModule.h>

#include <Extension\XExtensionMacros.h>
#include <Debugging\InvalidParameterHandler.h>

#include "RenderNull.h"

X_NAMESPACE_BEGIN(render)

render::IRender* CreateRender(ICore* pCore)
{
    X_UNUSED(pCore);
    LinkModule(pCore, "RenderNull");

    render::IRender* pRender = &render::g_NullRender;

    return pRender;
}

//////////////////////////////////////////////////////////////////////////
class XEngineModule_Render : public IEngineModule
{
    X_ENGINE_INTERFACE_SIMPLE(IEngineModule);

    X_ENGINE_GENERATE_SINGLETONCLASS(XEngineModule_Render, "Engine_RenderNull",
        0x7072e440, 0x4bf4, 0x4b09, 0xaf, 0xb5, 0xd8, 0x71, 0xeb, 0x3, 0xc8, 0xc3);

    //////////////////////////////////////////////////////////////////////////
    virtual const char* GetName(void) X_OVERRIDE
    {
        return "RenderNull";
    };

    //////////////////////////////////////////////////////////////////////////
    virtual bool Initialize(CoreGlobals& env, const CoreInitParams& initParams) X_OVERRIDE
    {
        X_UNUSED(initParams);

        env.pRender = CreateRender(env.pCore);
        return true;
    }

    virtual bool ShutDown(void) X_OVERRIDE
    {
        return true;
    }
};

X_ENGINE_REGISTER_CLASS(XEngineModule_Render);

XEngineModule_Render::XEngineModule_Render()
    = default;

XEngineModule_Render::~XEngineModule_Render()
    = default;

X_NAMESPACE_END
