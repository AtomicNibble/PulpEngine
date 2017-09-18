
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
> MatLibArena;

extern MatLibArena* g_MatLibArena;


#ifdef X_LIB
#define MATLIB_EXPORT
#else
#ifdef MAT_LIB_EXPORT
#define MATLIB_EXPORT X_EXPORT
#else
#define MATLIB_EXPORT X_IMPORT
#endif // !MAT_LIB_EXPORT
#endif // X_LIB


