#include "stdafx.h"

#include <ModuleExports.h> // needed for gEnv

#include <ICore.h>
#include <IEngineModule.h>

#include <Extension\XExtensionMacros.h>

X_USING_NAMESPACE;


class XEngineModule_ConverterLib : public IEngineModule
{
	X_ENGINE_INTERFACE_SIMPLE(IEngineModule);

	X_ENGINE_GENERATE_SINGLETONCLASS(XEngineModule_ConverterLib, "Engine_ConverterLib",
	0x4283dc03, 0x903, 0x43d4, 0x8b, 0xf8, 0x38, 0x40, 0x14, 0xf, 0x8d, 0xe5);


	virtual const char* GetName(void) X_OVERRIDE
	{
		return "ConverterLib";
	}

	virtual bool Initialize(SCoreGlobals& env, const SCoreInitParams& initParams) X_OVERRIDE
	{
		X_UNUSED(initParams);
		ICore* pCore = env.pCore;

		LinkModule(pCore, "ConverterLib");

		return true;
	}

	virtual bool ShutDown(void) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);
		return true;
	}
};

X_POTATO_REGISTER_CLASS(XEngineModule_ConverterLib);


XEngineModule_ConverterLib::XEngineModule_ConverterLib()
{
}

XEngineModule_ConverterLib::~XEngineModule_ConverterLib()
{
}

