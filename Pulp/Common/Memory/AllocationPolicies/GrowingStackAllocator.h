
#pragma once
#ifndef X_GROWINGSTACKALLOCATOR_H
#define X_GROWINGSTACKALLOCATOR_H

#include "Memory/MemoryAllocatorStatistics.h"

X_NAMESPACE_BEGIN(core)

class GrowingStackAllocator
{
    X_PACK_PUSH(4)
    struct BlockHeader
    {
#if X_ENABLE_STACK_ALLOCATOR_CHECK
        uint32_t AllocationID_;
#endif
        uint32_t allocationOffset_;
        uint32_t allocationSize_;
    };
    X_PACK_POP;

public:
    GrowingStackAllocator(size_t maxSizeInBytes, size_t granularity);

    ~GrowingStackAllocator(void);

    void* allocate(size_t size, size_t alignment, size_t offset);

    void free(void* ptr);
    void free(void* ptr, size_t size);

    void purge(void);

    inline size_t getSize(void* allocation) const;
    inline size_t usableSize(void* ptr) const;

    MemoryAllocatorStatistics getStatistics(void) const;

private:
    char* virtualStart_;
    char* virtualEnd_;
    char* physicalCurrent_;
    char* physicalEnd_;
    size_t granularity_;

#if X_ENABLE_STACK_ALLOCATOR_CHECK
    uint32_t allocationID_;
#endif

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    MemoryAllocatorStatistics statistics_;
#endif
};

#include "GrowingStackAllocator.inl"

X_NAMESPACE_END

#endif
