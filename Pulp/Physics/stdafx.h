
#pragma once



#include <EngineCommon.h>

#include "PxPhysicsAPI.h"


#define	PHYSX_VERSION_STRING	X_STRINGIZE(PX_PHYSICS_VERSION_MAJOR) "." X_STRINGIZE(PX_PHYSICS_VERSION_MINOR)

namespace physx
{
	class PxDefaultCpuDispatcher;
	class PxPhysics;
	class PxCooking;
	class PxScene;
	class PxGeometry;
	class PxMaterial;
	class PxRigidActor;
};


extern core::MemoryArenaBase* g_PhysicsArena;



#define PHYSX_DEFAULT_ALLOCATOR 0