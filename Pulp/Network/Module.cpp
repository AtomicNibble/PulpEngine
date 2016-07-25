#include "stdafx.h"
#include <ModuleExports.h>

#include <ICore.h>
#include <IEngineModule.h>

#include <Extension\XExtensionMacros.h>


X_USING_NAMESPACE;

NetworkArena* g_NetworkArena = nullptr;


namespace {
	core::MallocFreeAllocator g_NetworkAlloc;
}




//////////////////////////////////////////////////////////////////////////
class XEngineModule_Network : public IEngineModule
{
	X_POTATO_GENERATE_SINGLETONCLASS(XEngineModule_Network, "Engine_Network");

	//////////////////////////////////////////////////////////////////////////
	virtual const char *GetName() X_OVERRIDE { return "Network"; };

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize(SCoreGlobals &env, const SCoreInitParams &initParams) X_OVERRIDE
	{
		X_UNUSED(initParams);
		ICore* pCore = env.pCore;
	

		LinkModule(pCore, "Network");

		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);

		// kinky shit.
		g_NetworkArena = X_NEW(NetworkArena, gEnv->pArena, "NetworkArena")(&g_NetworkAlloc, "NetworkArena");



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

X_POTATO_REGISTER_CLASS(XEngineModule_Network);

XEngineModule_Network::XEngineModule_Network()
{
};

XEngineModule_Network::~XEngineModule_Network()
{

};
