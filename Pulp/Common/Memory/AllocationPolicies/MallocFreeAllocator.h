
#pragma once
#ifndef X_MALLOCFREEALLOCATOR_H
#define X_MALLOCFREEALLOCATOR_H

#include "Memory/MemoryAllocatorStatistics.h"

X_NAMESPACE_BEGIN(core)

class MallocFreeAllocator
{
    struct BlockHeader
    {
        void* originalAllocation_;
        size_t AllocationSize_;
        size_t originalSize_;
    };

public:
    static const size_t SIZE_OF_HEADER = sizeof(BlockHeader);

public:
    MallocFreeAllocator(void);

    void* allocate(size_t size, size_t alignment, size_t offset);

    void free(void* ptr);
    void free(void* ptr, size_t size);

    inline size_t getSize(void* allocation) const;
    inline size_t usableSize(void* ptr) const;

    MemoryAllocatorStatistics getStatistics(void) const;

private:
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    MemoryAllocatorStatistics statistics_;
#endif
};

#include "MallocFreeAllocator.inl"

X_NAMESPACE_END

#endif
