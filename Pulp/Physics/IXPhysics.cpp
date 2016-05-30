#include "stdafx.h"
#include <ModuleExports.h>

#include <ICore.h>
#include <IEngineModule.h>
#include <IPhysics.h>

#include <Extension\XExtensionMacros.h>

#include "XPhysics.h"

X_USING_NAMESPACE;


#include "Memory\BoundsCheckingPolicies\NoBoundsChecking.h"
#include "Memory\MemoryTaggingPolicies\NoMemoryTagging.h"
#include "Memory\MemoryTrackingPolicies\NoMemoryTracking.h"

typedef core::MemoryArena<
	core::MallocFreeAllocator,
	core::SingleThreadPolicy,
#if X_DEBUG
	core::SimpleBoundsChecking,
	core::SimpleMemoryTracking,
	core::SimpleMemoryTagging
#else
	core::NoBoundsChecking,
	core::NoMemoryTracking,
	core::NoMemoryTagging
#endif // !X_DEBUG
> PhysicsArena;



namespace {
	core::MallocFreeAllocator g_PhysicsAlloc;
}


core::MemoryArenaBase* g_PhysicsArena = nullptr;


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
		X_UNUSED(initParams);

		physics::IPhysics* pPhysics = nullptr;


		// kinky shit.
		g_PhysicsArena = X_NEW(PhysicsArena, gEnv->pArena, "PhysicsArena")(&g_PhysicsAlloc, "PhysicsArena");
		pPhysics = X_NEW(physics::XPhysics, g_PhysicsArena, "PhysicisSys");

		pPhysics->RegisterVars();
		pPhysics->RegisterCmds();

		if (!pPhysics->Init()) {
			X_DELETE(pPhysics, g_PhysicsArena);
			return false;
		}

		env.pPhysics = pPhysics;
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
