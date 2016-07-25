#include "stdafx.h"
#include <ModuleExports.h>

#include <ICore.h>
#include <IEngineModule.h>

#include <Extension\XExtensionMacros.h>

#include "ScriptSys.h"

X_USING_NAMESPACE;

#include <Memory\MemoryTrackingPolicies\FullMemoryTracking.h>
#include <Memory\MemoryTrackingPolicies\ExtendedMemoryTracking.h>

typedef core::MemoryArena<
	core::MallocFreeAllocator,
	core::SingleThreadPolicy,
	core::SimpleBoundsChecking,
	core::SimpleMemoryTracking,
	//	core::FullMemoryTracking,
	// core::ExtendedMemoryTracking,
	core::SimpleMemoryTagging
> ScriptArena;

// the allocator dose not check for leaks so it
// dose not need to go out of scope.
namespace {
	core::MallocFreeAllocator g_ScriptAlloc;
}

core::MemoryArenaBase* g_ScriptArena = nullptr;


//////////////////////////////////////////////////////////////////////////
class XEngineModule_Script : public IEngineModule
{
	X_POTATO_GENERATE_SINGLETONCLASS(XEngineModule_Script, "Engine_Script");
	//////////////////////////////////////////////////////////////////////////
	virtual const char* GetName(void) X_OVERRIDE{ return "Script"; };

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize(SCoreGlobals &env, const SCoreInitParams &initParams) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);
		X_UNUSED(initParams);

		script::IScriptSys *pScript = nullptr;

		g_ScriptArena = X_NEW_ALIGNED(ScriptArena, gEnv->pArena, "ScriptArena", X_ALIGN_OF(ScriptArena))(&g_ScriptAlloc, "ScriptArena");
		pScript = X_NEW_ALIGNED(script::XScriptSys, g_ScriptArena, "ScriptSys", X_ALIGN_OF(script::XScriptSys));

		env.pScriptSys = pScript;
		return true;
	}

	virtual bool ShutDown(void) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);

		X_DELETE_AND_NULL(g_ScriptArena, gEnv->pArena);


		return true;
	}
};



X_POTATO_REGISTER_CLASS(XEngineModule_Script);

XEngineModule_Script::XEngineModule_Script()
{
};

XEngineModule_Script::~XEngineModule_Script()
{
};