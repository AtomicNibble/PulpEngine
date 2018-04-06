#pragma once

#ifndef X_SIMPLEMEMORYTRACKING_H_
#define X_SIMPLEMEMORYTRACKING_H_

#if X_ENABLE_MEMORY_SIMPLE_TRACKING

X_NAMESPACE_BEGIN(core)

/// \ingroup Memory
/// \brief A class that implements a tracking policy for memory arenas.
/// \details This class implements the concepts of a tracking policy as expected by the MemoryArena class. It simply
/// counts the number of allocations, and reports the number of memory leaks detected without providing any further info.
///
/// This kind of tracker is extremely fast and causes no allocation overhead. Thus, it can be used to detect basic leaks
/// in all build configurations without having to sacrifice lots of performance. Once a leak has been detected, switching
/// to a more advanced tracker makes it easier to track down the location of a leak.
/// \sa NoMemoryTracking ExtendedMemoryTracking FullMemoryTracking
class SimpleMemoryTracking
{
public:
    /// A human-readable string literal containing the policy's type.
    static const char* const TYPE_NAME;

    /// Defines the amount of overhead that each allocation causes.
    static const size_t PER_ALLOCATION_OVERHEAD = 0;

    /// Default constructor.
    SimpleMemoryTracking(void);

    /// Default destructor, reporting the number of detected leaks.
    ~SimpleMemoryTracking(void);

    /// Increases the internal counter.
    inline void OnAllocation(void* memory, size_t originalSize, size_t internalSize, size_t alignment, size_t offset X_MEM_HUMAN_IDS_CB(const char* ID) X_MEM_HUMAN_IDS_CB(const char* typeName) X_SOURCE_INFO_MEM_CB(const SourceInfo& sourceInfo),
        const char* memoryArenaName);

    /// Decreases the internal counter.
    inline void OnDeallocation(void* memory);

private:
    unsigned int numAllocations_;
};

#include "SimpleMemoryTracking.inl"

X_NAMESPACE_END

#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING

#endif // !X_SIMPLEMEMORYTRACKING_H_
