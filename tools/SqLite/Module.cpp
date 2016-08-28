#include "stdafx.h"

#include <ModuleExports.h> // needed for gEnv

#include <ICore.h>
#include <IEngineModule.h>

#include <Extension\XExtensionMacros.h>

X_USING_NAMESPACE;


class XEngineModule_SqLite : public IEngineModule
{
	X_POTATO_GENERATE_SINGLETONCLASS(XEngineModule_SqLite, "Engine_SqLite");

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
