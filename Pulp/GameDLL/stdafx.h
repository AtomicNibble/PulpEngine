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
> GameArena;


extern GameArena* g_gameArena;

#include <Math\XAngles.h>

#include <IGame.h>
#include <IEntity.h>

#include "ECS\ComponentPool.h"
#include "ECS\Registry.h"


#include "Enity\EnityComponents.h"