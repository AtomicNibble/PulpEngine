#pragma once

X_NAMESPACE_BEGIN(physics)

// same for all configs.
#define PHYSX_SCENE_REQUIRES_LOCK 1
#define PHYSX_DEFAULT_ALLOCATOR 0

#if X_DEBUG

#elif X_RELEASE

#elif X_SUPER

#else
#error "unkonw config"
#endif

X_NAMESPACE_END