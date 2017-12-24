#include "stdafx.h"

#include <ModuleExports.h> // needed for gEnv

#include <ICore.h>
#include <IEngineModule.h>

#include <Extension\XExtensionMacros.h>

X_USING_NAMESPACE;


class XEngineModule_LinkerLib : public IEngineModule
{
	X_POTATO_INTERFACE_SIMPLE(IEngineModule);

	X_POTATO_GENERATE_SINGLETONCLASS(XEngineModule_LinkerLib, "Engine_LinkerLib",
		0x88ac2aa9, 0xcda5, 0x4d13, 0x87, 0x4, 0x9e, 0x6d, 0x8f, 0x2d, 0x9, 0xf2);


	virtual const char* GetName(void) X_OVERRIDE
	{
		return "LinkerLib";
	}

	virtual bool Initialize(SCoreGlobals& env, const SCoreInitParams& initParams) X_OVERRIDE
	{
		X_UNUSED(initParams);
		ICore* pCore = env.pCore;

		LinkModule(pCore, "LinkerLib");

		return true;
	}

	virtual bool ShutDown(void) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);
		return true;
	}
};

X_POTATO_REGISTER_CLASS(XEngineModule_LinkerLib);


XEngineModule_LinkerLib::XEngineModule_LinkerLib()
{
}

XEngineModule_LinkerLib::~XEngineModule_LinkerLib()
{
}

