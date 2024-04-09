#pragma once

#ifndef X_SIMPLEMEMORYTRACKING_H_
#define X_SIMPLEMEMORYTRACKING_H_

#if X_ENABLE_MEMORY_SIMPLE_TRACKING

X_NAMESPACE_BEGIN(core)

// A class that implements a tracking policy for memory arenas.
class SimpleMemoryTracking
{
public:
    static const char* const TYPE_NAME;
    static const size_t PER_ALLOCATION_OVERHEAD = 0;

    SimpleMemoryTracking(void);
    ~SimpleMemoryTracking(void);

    inline void OnAllocation(void* memory, size_t originalSize, size_t internalSize, size_t alignment, size_t offset X_MEM_HUMAN_IDS_CB(const char* ID) X_MEM_HUMAN_IDS_CB(const char* typeName) X_SOURCE_INFO_MEM_CB(const SourceInfo& sourceInfo),
        const char* memoryArenaName);

    inline void OnDeallocation(void* memory);

private:
    unsigned int numAllocations_;
};

#include "SimpleMemoryTracking.inl"

X_NAMESPACE_END

#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING

#endif // !X_SIMPLEMEMORYTRACKING_H_
