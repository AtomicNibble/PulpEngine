#include "stdafx.h"

#include <IPhysics.h>

#include <IConverterModule.h>
#include <Extension\XExtensionMacros.h>
#include <Util\UniquePointer.h>

#include "XPhysLib.h"

namespace
{
    core::MallocFreeAllocator g_PhysicsAlloc;
}

class XConverterLib_Phys : public IConverterModule
{
    // X_ENGINE_INTERFACE_SIMPLE(IConverterModule);
    X_ENGINE_INTERFACE_BEGIN()
    X_ENGINE_INTERFACE_ADD(IConverterModule)
    X_ENGINE_INTERFACE_END()

    X_ENGINE_GENERATE_SINGLETONCLASS(XConverterLib_Phys, "Engine_PhysLib",
        0x25f95d5f, 0xef87, 0x4b24, 0x95, 0xd2, 0x2b, 0xc, 0xb0, 0x76, 0xd5, 0x96);

    virtual const char* GetName(void) X_OVERRIDE
    {
        return "Phys";
    }

    virtual IConverter* Initialize(void) X_OVERRIDE
    {
        X_ASSERT_NOT_NULL(gEnv);
        X_ASSERT_NOT_NULL(gEnv->pArena);

        // you can't call init on both a engine module and a converter module instnace.
        if (g_PhysicsArena) {
            return nullptr;
        }

        g_PhysicsArena = X_NEW(PhysicsArena, gEnv->pArena, "PhysicsArena")(&g_PhysicsAlloc, "PhysicsArena");

        auto lib = core::makeUnique<physics::XPhysLib>(g_PhysicsArena, g_PhysicsArena);

        if (!lib->init()) {
            X_ERROR("Phys", "Failed to init lib");
            return nullptr;
        }

        return lib.release();
    }

    virtual bool ShutDown(IConverter* pCon) X_OVERRIDE
    {
        X_ASSERT_NOT_NULL(gEnv);
        X_ASSERT_NOT_NULL(gEnv->pArena);

        X_DELETE_AND_NULL(pCon, g_PhysicsArena);
        X_DELETE_AND_NULL(g_PhysicsArena, gEnv->pArena);
        return true;
    }
};

X_ENGINE_REGISTER_CLASS(XConverterLib_Phys);

XConverterLib_Phys::XConverterLib_Phys()
{
}

XConverterLib_Phys::~XConverterLib_Phys()
{
}
