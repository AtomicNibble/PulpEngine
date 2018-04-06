#pragma once

#ifndef X_MEMORYARENASTATISTICS_H_
#define X_MEMORYARENASTATISTICS_H_

#include "Memory/MemoryAllocatorStatistics.h"

X_NAMESPACE_BEGIN(core)

struct MemoryArenaStatistics
{
    // general
    const char* arenaName_;                ///< A human-readable string describing the name of the arena.
    const char* arenaType_;                ///< A human-readable string describing the type of the arena.
    const char* threadPolicyType_;         ///< A human-readable string describing the type of the thread policy.
    const char* boundsCheckingPolicyType_; ///< A human-readable string describing the type of the bounds checking policy.
    const char* memoryTrackingPolicyType_; ///< A human-readable string describing the type of the memory tracking policy.
    const char* memoryTaggingPolicyType_;  ///< A human-readable string describing the type of the memory tagging policy.

    // allocator
    MemoryAllocatorStatistics allocatorStatistics_; ///< Individual allocator statistics.

    // overhead
    unsigned int trackingOverhead_;       ///< The overhead caused by memory tracking.
    unsigned int boundsCheckingOverhead_; ///< The overhead caused by bounds checking.
};

X_NAMESPACE_END

#endif // X_MEMORYARENASTATISTICS_H_
