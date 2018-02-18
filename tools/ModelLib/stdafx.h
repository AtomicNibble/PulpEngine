
#pragma once


#include <EngineCommon.h>


#include "Memory\BoundsCheckingPolicies\NoBoundsChecking.h"
#include "Memory\MemoryTaggingPolicies\NoMemoryTagging.h"
#include "Memory\MemoryTrackingPolicies\NoMemoryTracking.h"
#include "Memory\ThreadPolicies\MultiThreadPolicy.h"

typedef core::MemoryArena<
	core::MallocFreeAllocator,
	core::MultiThreadPolicy<core::Spinlock>,
#if X_DEBUG
	core::SimpleBoundsChecking,
	core::SimpleMemoryTracking,
	core::SimpleMemoryTagging
#else
	core::NoBoundsChecking,
	core::NoMemoryTracking,
	core::NoMemoryTagging
#endif // !X_DEBUG
> ModelLibrena;

extern ModelLibrena* g_ModelLibArena;



#ifdef X_LIB
#define MODELLIB_EXPORT
#else
#ifdef MODEL_LIB_EXPORT
#define MODELLIB_EXPORT X_EXPORT
#else
#define MODELLIB_EXPORT X_IMPORT
#endif // !MODEL_LIB_EXPORT
#endif // X_LIB


