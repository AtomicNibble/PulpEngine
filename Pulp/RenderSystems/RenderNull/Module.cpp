#include "stdafx.h"
#include <ModuleExports.h>

#include <ICore.h>
#include <IRender.h>
#include <IEngineModule.h>

#include <Extension\XExtensionMacros.h>
#include <Debugging\InvalidParameterHandler.h>


#include "RenderNull.h"

X_NAMESPACE_BEGIN(render)


render::IRender* CreateRender(ICore *pCore)
{
	LinkModule(pCore, "RenderNull");

	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pArena);

	render::IRender* pRender = &render::g_NullRender;

	return pRender;
}


//////////////////////////////////////////////////////////////////////////
class XEngineModule_Render : public IEngineModule
{
	X_POTATO_INTERFACE_SIMPLE(IEngineModule);

	X_POTATO_GENERATE_SINGLETONCLASS(XEngineModule_Render, "Engine_RenderNull",
	0x7072e440, 0x4bf4, 0x4b09, 0xaf, 0xb5, 0xd8, 0x71, 0xeb, 0x3, 0xc8, 0xc3);


	//////////////////////////////////////////////////////////////////////////
	virtual const char* GetName(void) X_OVERRIDE{ return "RenderNull"; };

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

X_POTATO_REGISTER_CLASS(XEngineModule_Render);

XEngineModule_Render::XEngineModule_Render()
{
};

XEngineModule_Render::~XEngineModule_Render()
{
};



X_NAMESPACE_END
