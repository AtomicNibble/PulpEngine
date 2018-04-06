#pragma once

#include <EngineCommon.h>

#define IPGAMEDLL_EXPORTS

#include <Memory\MemoryTrackingPolicies\FullMemoryTracking.h>
#include <Memory\MemoryTrackingPolicies\ExtendedMemoryTracking.h>

typedef core::MemoryArena<
    core::MallocFreeAllocator,
    core::SingleThreadPolicy,
#if X_ENABLE_MEMORY_DEBUG_POLICIES
    core::SimpleBoundsChecking,
    core::SimpleMemoryTracking,
    core::SimpleMemoryTagging
#else
    core::NoBoundsChecking,
    core::NoMemoryTracking,
    core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
    >
    GameArena;

extern GameArena* g_gameArena;

#include <../../tools/ModelLib/ModelLib.h>
#include <../../tools/AnimLib/AnimLib.h>

X_LINK_ENGINE_LIB("ModelLib");
X_LINK_ENGINE_LIB("AnimLib");

#include <Math\XAngles.h>

#include <IGame.h>
#include <IWeapon.h>
#include <IEntity.h>
#include <ISound.h>

#include <IPrimativeContext.h>
#include <IFont.h>

#include "ECS\ComponentPool.h"
#include "ECS\Registry.h"

#include "Enity\EnityComponents.h"
