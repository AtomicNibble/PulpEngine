#pragma once

#ifndef X_STACKALLOCATOR_H
#define X_STACKALLOCATOR_H

#include "Memory/MemoryAllocatorStatistics.h"

X_NAMESPACE_BEGIN(core)

class StackAllocator
{
    struct BlockHeader
    {
#if X_ENABLE_STACK_ALLOCATOR_CHECK
        uint32_t AllocationID_;
#endif
        size_t allocationOffset_;
        size_t AllocationSize_;
    };

public:
    StackAllocator(void* start, void* end);

    void* allocate(size_t size, size_t alignment, size_t offset);

    void free(void* ptr);
    void free(void* ptr, size_t size);

    inline size_t getSize(void* allocation) const;
    inline size_t usableSize(void* allocation) const;

    MemoryAllocatorStatistics getStatistics(void) const;

private:
    char* start_;
    char* end_;
    char* current_;

#if X_ENABLE_STACK_ALLOCATOR_CHECK
    uint32_t allocationID_;
#endif

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    MemoryAllocatorStatistics statistics_;
#endif
};

#include "StackAllocator.inl"

X_NAMESPACE_END

#endif
