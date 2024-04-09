#pragma once

#ifndef X_MEMORYARENA_H_
#define X_MEMORYARENA_H_

#include "Memory/MemoryArenaBase.h"
#include "Util/BitUtil.h"

X_NAMESPACE_BEGIN(core)

template<class AllocationPolicy, class ThreadPolicy, class BoundsCheckingPolicy, class MemoryTrackingPolicy, class MemoryTaggingPolicy>
class MemoryArena : public MemoryArenaBase
{
public:
    // A simple typedef that introduces the template type into this class.
    typedef AllocationPolicy AllocationPolicy;

    // A simple typedef that introduces the template type into this class.
    typedef ThreadPolicy ThreadPolicy;

    // A simple typedef that introduces the template type into this class.
    typedef BoundsCheckingPolicy BoundsCheckingPolicy;

    // A simple typedef that introduces the template type into this class.
    typedef MemoryTrackingPolicy MemoryTrackingPolicy;

    // A simple typedef that introduces the template type into this class.
    typedef MemoryTaggingPolicy MemoryTaggingPolicy;

    static const bool IS_THREAD_SAFE = ThreadPolicy::IS_THREAD_SAFE;

    MemoryArena(AllocationPolicy* allocator, const char* name);

    // Empty destructor.
    virtual ~MemoryArena(void);

    // Allocates raw memory that satisfies the alignment requirements.
    // All arguments provided for debugging purposes are passed along to the respective policies.
    virtual void* allocate(size_t size, size_t alignment, size_t offset X_MEM_HUMAN_IDS_CB(const char* ID) X_MEM_HUMAN_IDS_CB(const char* typeName) X_SOURCE_INFO_MEM_CB(const SourceInfo& sourceInfo)) X_FINAL;

    void free(void* ptr) X_FINAL;
    void free(void* ptr, size_t size) X_FINAL;

    size_t getSize(void* ptr) X_FINAL;

    size_t usableSize(void* ptr) const X_FINAL;

    // Returns statistics regarding the allocations made by the memory arena.
    MemoryArenaStatistics getStatistics(void) const X_FINAL;

    MemoryAllocatorStatistics getAllocatorStatistics(bool children = false) const X_FINAL;

    bool isThreadSafe(void) const X_FINAL;

    // Returns the memory requirement for an allocation of size, taking into account bounds checking and other potential overhead.
    static constexpr inline size_t getMemoryRequirement(size_t size);

    // Returns the alignment requirement for an allocation requiring alignment, taking into account bounds checking and other potential overhead.
    static constexpr inline size_t getMemoryAlignmentRequirement(size_t alignment);

    // Returns the offset requirement for an allocation, taking into account bounds checking and other potential overhead.
    static constexpr inline size_t getMemoryOffsetRequirement(void);

private:
    X_NO_COPY(MemoryArena);
    X_NO_ASSIGN(MemoryArena);

    AllocationPolicy* allocator_;
    ThreadPolicy threadGuard_;
    BoundsCheckingPolicy boundsChecker_;
    MemoryTrackingPolicy memoryTracker_;
    MemoryTaggingPolicy memoryTagger_;

    const char* name_;

#if X_ENABLE_MEMORY_ARENA_STATISTICS
    MemoryArenaStatistics statistics_;
#endif
};

#include "MemoryArena.inl"

X_NAMESPACE_END

#endif
