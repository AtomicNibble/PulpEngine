#pragma once

#ifndef X_MEMORYARENASTATISTICS_H_
#define X_MEMORYARENASTATISTICS_H_

#include "Memory/MemoryAllocatorStatistics.h"


X_NAMESPACE_BEGIN(core)


struct MemoryArenaStatistics
{
	// general
	const char* m_arenaName;									///< A human-readable string describing the name of the arena.
	const char* m_arenaType;									///< A human-readable string describing the type of the arena.
	const char* m_threadPolicyType;								///< A human-readable string describing the type of the thread policy.
	const char* m_boundsCheckingPolicyType;						///< A human-readable string describing the type of the bounds checking policy.
	const char* m_memoryTrackingPolicyType;						///< A human-readable string describing the type of the memory tracking policy.
	const char* m_memoryTaggingPolicyType;						///< A human-readable string describing the type of the memory tagging policy.

	// allocator
	MemoryAllocatorStatistics m_allocatorStatistics;			///< Individual allocator statistics.

	// overhead
	unsigned int m_trackingOverhead;							///< The overhead caused by memory tracking.
	unsigned int m_boundsCheckingOverhead;						///< The overhead caused by bounds checking.
};

X_NAMESPACE_END


#endif // X_MEMORYARENASTATISTICS_H_
