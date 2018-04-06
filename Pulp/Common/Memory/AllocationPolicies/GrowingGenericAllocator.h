#pragma once
#ifndef X_GROWINGGENERICALLOCATOR_H
#define X_GROWINGGENERICALLOCATOR_H

#include "Memory/AllocationPolicies/GrowingMicroAllocator.h"
#include "Memory/AllocationPolicies/GrowingBlockAllocator.h"
#include "Memory/MemoryAllocatorStatistics.h"

X_NAMESPACE_BEGIN(core)

class GrowingGenericAllocator
{
public:
    GrowingGenericAllocator(uint32_t maxSizeInBytesPerPool, uint32_t growSize, size_t maxAlignment, size_t offset);

    X_INLINE void* allocate(size_t size, size_t alignment, size_t offset);

    X_INLINE void free(void* ptr);
    X_INLINE void free(void* ptr, size_t size);

    X_INLINE size_t getSize(void* ptr) const;
    X_INLINE size_t usableSize(void* allocation) const;

    MemoryAllocatorStatistics getStatistics(void) const;

private:
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    void updateStatistics(void);
#endif

    X_NO_COPY(GrowingGenericAllocator);
    X_NO_ASSIGN(GrowingGenericAllocator);

    GrowingMicroAllocator microAllocator_;
    GrowingBlockAllocator blockAllocator_;

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    MemoryAllocatorStatistics statistics_;
#endif
};

#include "GrowingGenericAllocator.inl"

X_NAMESPACE_END

#endif
