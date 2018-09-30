#pragma once

#include <EngineCommon.h>

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
    AssetDBArena;

extern AssetDBArena* g_AssetDBArena;

#include "../SqLite/SqlLib.h"

#ifdef X_LIB
#define ASSETDBLIB_EXPORT
#else
#define ASSETDBLIB_EXPORT X_EXPORT
#endif // X_LIB