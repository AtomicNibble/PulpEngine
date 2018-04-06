#pragma once

#include <IPhysics.h>

#include "Util/Allocator.h"

X_NAMESPACE_BEGIN(physics)

class PhysCooking : public IPhysicsCooking
{
public:
    PhysCooking(core::MemoryArenaBase* arena);
    ~PhysCooking() X_FINAL;

    bool init(const physx::PxTolerancesScale& scale, physx::PxFoundation& foundation, CookingMode::Enum mode);
    void shutDown(void);

    // IPhysicsCooking
    bool cookingSupported(void) const X_FINAL;
    virtual bool setCookingMode(CookingMode::Enum mode) X_FINAL;

    bool cookTriangleMesh(const TriangleMeshDesc& desc, DataArr& dataOut, CookFlags flags) X_FINAL;
    bool cookConvexMesh(const TriangleMeshDesc& desc, DataArr& dataOut, CookFlags flags) X_FINAL;
    bool cookConvexMesh(const ConvexMeshDesc& desc, DataArr& dataOut, CookFlags flags) X_FINAL;
    bool cookHeightField(const HeightFieldDesc& desc, DataArr& dataOut) X_FINAL;
    // ~IPhysicsCooking

private:
    static void setCookingParamsForMode(physx::PxCookingParams& params, CookingMode::Enum mode);

private:
    core::MemoryArenaBase* arena_;
    PhysxArenaAllocator physAllocator_;

    physx::PxCooking* pCooking_;
    physx::PxTolerancesScale scale_;
};

X_NAMESPACE_END