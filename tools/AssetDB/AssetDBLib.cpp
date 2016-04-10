#include "stdafx.h"

#include <ModuleExports.h> // needed for gEnv

#include <ICore.h>
#include <IEngineModule.h>

#include <Extension\XExtensionMacros.h>

X_USING_NAMESPACE;

namespace
{

	core::MallocFreeAllocator g_AssetDBAlloc;

} // namespace

AssetDBArena* g_AssetDBArena = nullptr;


class XEngineModule_AssetDB : public IEngineModule
{
	X_POTATO_GENERATE_SINGLETONCLASS(XEngineModule_AssetDB, "Engine_AssetDB");

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

X_POTATO_REGISTER_CLASS(XEngineModule_AssetDB);


XEngineModule_AssetDB::XEngineModule_AssetDB()
{
}

XEngineModule_AssetDB::~XEngineModule_AssetDB()
{
}
