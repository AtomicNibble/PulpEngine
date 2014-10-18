#pragma once


#include <EngineCommon.h>

#define IPGAMEDLL_EXPORTS


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
> GameArena;


extern GameArena* g_gameArena;
