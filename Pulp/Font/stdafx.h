#pragma once


#include <EngineCommon.h>


#define IPFONT_EXPORTS

// we don't need font rendering for server.
#if defined(X_DEDICATED_SERVER) && !defined(X_USE_NULLFONT)
 #define X_USE_NULLFONT
#endif // X_DEDICATED_SERVER


#include <Platform.h>

#include <IConsole.h>
#include <IFont.h>


#include <Memory\MemoryTrackingPolicies\FullMemoryTracking.h>
#include <Memory\MemoryTrackingPolicies\ExtendedMemoryTracking.h>

typedef core::MemoryArena<
	core::MallocFreeAllocator,
	core::SingleThreadPolicy,
	core::SimpleBoundsChecking,
	core::SimpleMemoryTracking,
//	core::FullMemoryTracking,
//	core::ExtendedMemoryTracking,
	core::SimpleMemoryTagging
> FontArena;


extern FontArena* g_fontArena;

