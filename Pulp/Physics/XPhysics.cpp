#include "stdafx.h"
#include <ModuleExports.h>

#include <ICore.h>
#include <IEngineModule.h>

#include <Extension\XExtensionMacros.h>


X_USING_NAMESPACE;

PhysicsArena* g_PhysicsArena = nullptr;


namespace {
	core::MallocFreeAllocator g_PhysicsAlloc;
}




//////////////////////////////////////////////////////////////////////////
class XEngineModule_Physics : public IEngineModule
{
	X_POTATO_GENERATE_SINGLETONCLASS(XEngineModule_Physics, "Engine_Physics");

	//////////////////////////////////////////////////////////////////////////
	virtual const char *GetName() X_OVERRIDE { return "Physics"; };

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize(SCoreGlobals &env, const SCoreInitParams &initParams) X_OVERRIDE
	{
		X_UNUSED(initParams);
		ICore* pCore = env.pCore;


		LinkModule(pCore, "Physics");

		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);

		// kinky shit.
		g_PhysicsArena = X_NEW(PhysicsArena, gEnv->pArena, "PhysicsArena")(&g_PhysicsAlloc, "PhysicsArena");


		return true;
	}

	virtual bool ShutDown(void) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);

		X_DELETE_AND_NULL(g_PhysicsArena, gEnv->pArena);
		return true;
	}
};

X_POTATO_REGISTER_CLASS(XEngineModule_Physics);

XEngineModule_Physics::XEngineModule_Physics()
{
};

XEngineModule_Physics::~XEngineModule_Physics()
{

};
