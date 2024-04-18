#include "stdafx.h"

#include <ModuleExports.h> // needed for gEnv

#include <ICore.h>
#include <IEngineModule.h>

#include <Extension\XExtensionMacros.h>

X_USING_NAMESPACE;

X_LINK_ENGINE_LIB("SqLite")

#ifdef X_LIB

X_FORCE_LINK_FACTORY("XEngineModule_SqLite");

#endif // X_LIB

class XEngineModule_ConverterLib : public IEngineModule
{
    X_ENGINE_INTERFACE_SIMPLE(IEngineModule);

    X_ENGINE_GENERATE_SINGLETONCLASS(XEngineModule_ConverterLib, "Engine_ConverterLib",
        0x4283dc03, 0x903, 0x43d4, 0x8b, 0xf8, 0x38, 0x40, 0x14, 0xf, 0x8d, 0xe5);

    virtual const char* GetName(void) X_OVERRIDE
    {
        return "ConverterLib";
    }

    virtual bool Initialize(CoreGlobals& env, const CoreInitParams& initParams) X_OVERRIDE
    {
        X_UNUSED(initParams);
        ICore* pCore = env.pCore;

        LinkModule(pCore, "ConverterLib");

        if (!pCore->InitializeLoadedEngineModule(X_ENGINE_OUTPUT_PREFIX "SqLite", "Engine_SqLite")) {
            return false;
        }

        return true;
    }

    virtual bool ShutDown(void) X_OVERRIDE
    {
        X_ASSERT_NOT_NULL(gEnv);
        X_ASSERT_NOT_NULL(gEnv->pArena);
        return true;
    }
};

X_ENGINE_REGISTER_CLASS(XEngineModule_ConverterLib);

XEngineModule_ConverterLib::XEngineModule_ConverterLib()
{
}

XEngineModule_ConverterLib::~XEngineModule_ConverterLib()
{
}
