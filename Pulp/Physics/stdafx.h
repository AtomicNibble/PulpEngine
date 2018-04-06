
#pragma once

#include <EngineCommon.h>

#include "PxPhysicsAPI.h"

#define PHYSX_VERSION_STRING X_STRINGIZE(PX_PHYSICS_VERSION_MAJOR) \
"." X_STRINGIZE(PX_PHYSICS_VERSION_MINOR)

namespace physx
{
    class PxDefaultCpuDispatcher;
    class PxPhysics;
    class PxCooking;
    class PxScene;
    class PxGeometry;
    class PxMaterial;
    class PxRigidActor;
}; // namespace physx

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
    >
    PhysicsArena;

extern core::MemoryArenaBase* g_PhysicsArena;

#include "Util/Config.h"
#include "Util/SceneLock.h"
