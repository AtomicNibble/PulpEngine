#pragma once


#include <IPhysics.h>

X_NAMESPACE_BEGIN(physics)

class PhysCooking : public IPhysicsCooking
{
public:
	PhysCooking(core::MemoryArenaBase* arena);
	~PhysCooking() X_FINAL;

	bool init(const physx::PxTolerancesScale& scale, physx::PxFoundation& foundation);

	// IPhysicsCooking
	bool cookingSupported(void) const X_FINAL;

	bool cookTriangleMesh(const TriangleMeshDesc& desc, DataArr& dataOut) X_FINAL;
	bool cookConvexMesh(const ConvexMeshDesc& desc, DataArr& dataOut) X_FINAL;
	bool cookHeightField(const HeightFieldDesc& desc, DataArr& dataOut) X_FINAL;
	// ~IPhysicsCooking


private:
	core::MemoryArenaBase*	arena_;
	physx::PxCooking*		pCooking_;
};

X_NAMESPACE_END