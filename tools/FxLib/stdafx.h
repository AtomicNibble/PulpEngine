#pragma once

#include <EngineCommon.h>
#include <IEffect.h>


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
> FxLibArena;

extern FxLibArena* g_FxLibArena;



#ifdef X_LIB
#define FXLIB_EXPORT
#else
#ifdef FX_LIB_EXPORT
#define FXLIB_EXPORT X_EXPORT
#else
#define FXLIB_EXPORT X_IMPORT
#endif // !FX_LIB_EXPORT
#endif // X_LIB

