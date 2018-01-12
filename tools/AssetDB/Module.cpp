#include "stdafx.h"

#include <ModuleExports.h> // needed for gEnv

#include <ICore.h>
#include <IEngineModule.h>

#include <Extension\XExtensionMacros.h>

X_USING_NAMESPACE;


X_LINK_LIB("engine_SqLite")

#ifdef X_LIB

X_FORCE_LINK_FACTORY("XEngineModule_SqLite");

#endif // !X_LIB

namespace
{

	core::MallocFreeAllocator g_AssetDBAlloc;

} // namespace

AssetDBArena* g_AssetDBArena = nullptr;


class XEngineModule_AssetDB : public IEngineModule
{
	X_ENGINE_INTERFACE_SIMPLE(IEngineModule);

	X_ENGINE_GENERATE_SINGLETONCLASS(XEngineModule_AssetDB, "Engine_AssetDB",
	0x1f67e4b3, 0xa3b0, 0x4eaf, 0x85, 0x99, 0x9f, 0x82, 0xb0, 0x2e, 0x77, 0xc2);


	virtual const char* GetName(void) X_OVERRIDE
	{
		return "AssetDB";
	}

	virtual bool Initialize(SCoreGlobals& env, const SCoreInitParams& initParams) X_OVERRIDE
	{
		X_UNUSED(initParams);
		ICore* pCore = env.pCore;

		LinkModule(pCore, "AssetDB");
		
		g_AssetDBArena = X_NEW(AssetDBArena, gEnv->pArena, "AssetDBArena")(&g_AssetDBAlloc, "AssetDBArena");


		// Link the SqLite here?
		if (!pCore->IntializeLoadedEngineModule("Engine_SqLite", "Engine_SqLite")) {
			return false;
		}

		return true;
	}

	virtual bool ShutDown(void) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);

		X_DELETE_AND_NULL(g_AssetDBArena, gEnv->pArena);
		return true;
	}
};

X_ENGINE_REGISTER_CLASS(XEngineModule_AssetDB);


XEngineModule_AssetDB::XEngineModule_AssetDB()
{
}

XEngineModule_AssetDB::~XEngineModule_AssetDB()
{
}
