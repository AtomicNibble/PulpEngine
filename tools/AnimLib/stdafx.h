
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
> AnimLibArena;

extern AnimLibArena* g_AnimLibArena;


#ifdef X_LIB
#define ANIMLIB_EXPORT
#else
#ifdef ANIM_LIB_EXPORT
#define ANIMLIB_EXPORT X_EXPORT
#else
#define ANIMLIB_EXPORT X_IMPORT
#endif // !ANIM_LIB_EXPORT
#endif // X_LIB


