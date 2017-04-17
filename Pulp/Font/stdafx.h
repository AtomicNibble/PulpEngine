#pragma once


#include <EngineCommon.h>


#define IPFONT_EXPORTS

// we don't need font rendering for server.
#if defined(X_DEDICATED_SERVER) && !defined(X_USE_NULLFONT)
 #define X_USE_NULLFONT
#endif // X_DEDICATED_SERVER


#include <Core\Platform.h>

// some common used files.
#include <Util\ReferenceCounted.h>
#include <Util\UniquePointer.h>
#include <Threading\CriticalSection.h>

#include <IConsole.h>
#include <IFont.h>

#include "Constants.h"
#include "FontTypes.h"

#include <Memory\MemoryTrackingPolicies\FullMemoryTracking.h>
#include <Memory\MemoryTrackingPolicies\ExtendedMemoryTracking.h>
#include <Memory\ThreadPolicies\MultiThreadPolicy.h>

typedef core::MemoryArena<
	core::MallocFreeAllocator,
	core::MultiThreadPolicy<core::CriticalSection>,
#if X_ENABLE_MEMORY_DEBUG_POLICIES
	core::SimpleBoundsChecking,
	core::SimpleMemoryTracking,
	core::SimpleMemoryTagging
#else
	core::NoBoundsChecking,
	core::NoMemoryTracking,
	core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
> FontArena;


extern FontArena* g_fontArena;


#if X_DEBUG
X_LINK_LIB("freetype265d");
#else
X_LINK_LIB("freetype265");
#endif // !X_DEBUG

