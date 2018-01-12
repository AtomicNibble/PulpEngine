#include "stdafx.h"

#include <ModuleExports.h> // needed for gEnv

#include <ICore.h>
#include <IEngineModule.h>

#include <Extension\XExtensionMacros.h>

X_USING_NAMESPACE;


class XEngineModule_SqLite : public IEngineModule
{
	X_ENGINE_INTERFACE_SIMPLE(IEngineModule);

	X_ENGINE_GENERATE_SINGLETONCLASS(XEngineModule_SqLite, "Engine_SqLite",
		0xcf1a39a6, 0xd24d, 0x43e2,  0x89, 0xb0, 0xd6, 0xcf, 0xd5, 0x8c, 0xed, 0xdb );

	virtual const char* GetName(void) X_OVERRIDE
	{
		return "SqLite";
	}

	virtual bool Initialize(SCoreGlobals& env, const SCoreInitParams& initParams) X_OVERRIDE
	{
		X_UNUSED(initParams);
		ICore* pCore = env.pCore;

		LinkModule(pCore, "SqLite");

		return true;
	}

	virtual bool ShutDown(void) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);

		return true;
	}
};

X_POTATO_REGISTER_CLASS(XEngineModule_SqLite);


XEngineModule_SqLite::XEngineModule_SqLite()
{
}

XEngineModule_SqLite::~XEngineModule_SqLite()
{
}
