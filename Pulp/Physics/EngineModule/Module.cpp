#include "stdafx.h"
#include <ModuleExports.h>

#include <ICore.h>
#include <IEngineModule.h>
#include <IPhysics.h>

#include <Extension\XExtensionMacros.h>

#include "XPhysics.h"

X_USING_NAMESPACE;

namespace
{
    core::MallocFreeAllocator g_PhysicsAlloc;
}

core::MemoryArenaBase* g_PhysicsArena = nullptr;

//////////////////////////////////////////////////////////////////////////
class XEngineModule_Physics : public IEngineModule
{
    X_ENGINE_INTERFACE_SIMPLE(IEngineModule);

    X_ENGINE_GENERATE_SINGLETONCLASS(XEngineModule_Physics, "Engine_Physics",
        0xdd6730a7, 0x51f2, 0x42bc, 0x82, 0x71, 0xae, 0xe0, 0xc1, 0x85, 0xba, 0x71);

    //////////////////////////////////////////////////////////////////////////
    virtual const char* GetName() X_OVERRIDE
    {
        return "Physics";
    };

    //////////////////////////////////////////////////////////////////////////
    virtual bool Initialize(SCoreGlobals& env, const SCoreInitParams& initParams) X_OVERRIDE
    {
        X_UNUSED(initParams);
        ICore* pCore = env.pCore;

        LinkModule(pCore, "Physics");

        X_ASSERT_NOT_NULL(gEnv);
        X_ASSERT_NOT_NULL(gEnv->pArena);
        X_ASSERT_NOT_NULL(gEnv->pJobSys);
        X_UNUSED(initParams);

        physics::IPhysics* pPhysics = nullptr;

        if (g_PhysicsArena) {
            return nullptr;
        }

        // kinky shit.
        g_PhysicsArena = X_NEW(PhysicsArena, gEnv->pArena, "PhysicsArena")(&g_PhysicsAlloc, "PhysicsArena");
        pPhysics = X_NEW(physics::XPhysics, g_PhysicsArena, "PhysicisSys")(8, env.pJobSys, g_PhysicsArena);

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

X_ENGINE_REGISTER_CLASS(XEngineModule_Physics);

XEngineModule_Physics::XEngineModule_Physics() 
{

}

XEngineModule_Physics::~XEngineModule_Physics() 
{

}
