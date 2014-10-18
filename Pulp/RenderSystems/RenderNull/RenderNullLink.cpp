#include "stdafx.h"
#include <ModuleExports.h>

#include <ICore.h>
#include <IRender.h>
#include <IEngineModule.h>

#include <Extension\XExtensionMacros.h>
#include <Debugging\InvalidParameterHandler.h>


#include "RenderNull.h"

X_USING_NAMESPACE;


render::IRender* CreateRender(ICore *pCore)
{
	LinkModule(pCore, "RenderNull");

	render::IRender* pRender = &render::g_NullRender;

	return pRender;
}


//////////////////////////////////////////////////////////////////////////
class XEngineModule_Render : public IEngineModule
{
	X_GOAT_GENERATE_SINGLETONCLASS(XEngineModule_Render, "Engine_RenderNull");
	//////////////////////////////////////////////////////////////////////////
	virtual const char *GetName() X_OVERRIDE{ return "RenderNull"; };

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize(SCoreGlobals& env, const SCoreInitParams& initParams) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);
		X_UNUSED(initParams);

		ICore* pCore = env.pCore;
		render::IRender *pRender = 0;

		pRender = CreateRender(pCore);

		env.pRender = pRender;
		return true;
	}

	virtual bool ShutDown(void) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);


		return true;
	}
};

X_GOAT_REGISTER_CLASS(XEngineModule_Render);

XEngineModule_Render::XEngineModule_Render()
{
};

XEngineModule_Render::~XEngineModule_Render()
{
};

