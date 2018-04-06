#pragma once

#include <PxSceneLock.h>

X_NAMESPACE_BEGIN(physics)

#if PHYSX_SCENE_REQUIRES_LOCK

#define PHYS_SCENE_READ_LOCK(pScene) physx::PxSceneReadLock scopedLock(*pScene, __FILE__, __LINE__);
#define PHYS_SCENE_WRITE_LOCK(pScene) physx::PxSceneWriteLock scopedLock(*pScene, __FILE__, __LINE__);

#else

#define PHYS_SCENE_READ_LOCK(pScene) X_UNUSED(pScene);
#define PHYS_SCENE_WRITE_LOCK(pScene) X_UNUSED(pScene);

#endif // !PHYSX_SCENE_REQUIRES_LOCK

X_NAMESPACE_END