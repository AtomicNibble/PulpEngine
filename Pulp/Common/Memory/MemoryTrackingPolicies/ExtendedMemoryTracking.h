#pragma once

#ifndef X_EXTENDEDMEMORYTRACKING_H_
#define X_EXTENDEDMEMORYTRACKING_H_

#if X_ENABLE_MEMORY_DEBUG_POLICIES_DEFS

#include "Memory/HeapArea.h"
#include "Memory/MemoryArena.h"
#include "Memory/AllocationPolicies/LinearAllocator.h"
#include "Memory/ThreadPolicies/SingleThreadPolicy.h"
#include "Memory/BoundsCheckingPolicies/NoBoundsChecking.h"
#include "Memory/MemoryTaggingPolicies/NoMemoryTagging.h"
#include "Memory/MemoryTrackingPolicies/NoMemoryTracking.h"
// #include "Memory/MemoryTrackingPolicies/SimpleMemoryTracking.h"
#include "Containers\HashMap.h"
// #include "Containers\HashMapHashFunctions.h"

X_NAMESPACE_BEGIN(core)

// A class that implements a tracking policy for memory arenas.
class ExtendedMemoryTracking
{
    // The data stored for each allocation.
    struct AllocationData
    {
        AllocationData(size_t originalSize, size_t internalSize X_MEM_HUMAN_IDS_CB(const char* ID) X_MEM_HUMAN_IDS_CB(const char* typeName) X_SOURCE_INFO_MEM_CB(const SourceInfo& sourceInfo), const char* memoryArenaName);

        size_t originalSize_;
        size_t internalSize_;
#if X_ENABLE_MEMORY_HUMAN_IDS
        const char* ID_;
        const char* typeName_;
#endif // !X_ENABLE_MEMORY_HUMAN_IDS
#if X_ENABLE_MEMORY_SOURCE_INFO
        SourceInfo sourceInfo_;
#endif // !X_ENABLE_MEMORY_SOURCE_INFO
        const char* memoryArenaName_;

    private:
        X_NO_ASSIGN(AllocationData);
    };

    typedef HashMap<void*, AllocationData> AllocationTable;
    typedef MemoryArena<LinearAllocator, SingleThreadPolicy, NoBoundsChecking, NoMemoryTracking, NoMemoryTagging> LinearArena;

public:
    static const char* const TYPE_NAME;
    static const size_t PER_ALLOCATION_OVERHEAD = AllocationTable::PER_ENTRY_SIZE;

    ExtendedMemoryTracking(void);
    ~ExtendedMemoryTracking(void);

    void OnAllocation(void* memory, size_t originalSize, size_t internalSize, size_t alignment, size_t offset X_MEM_HUMAN_IDS_CB(const char* ID) X_MEM_HUMAN_IDS_CB(const char* typeName) X_SOURCE_INFO_MEM_CB(const SourceInfo& sourceInfo),
        const char* memoryArenaName);

    void OnDeallocation(void* memory);

private:
    X_NO_COPY(ExtendedMemoryTracking);
    X_NO_ASSIGN(ExtendedMemoryTracking);

    unsigned int numAllocations_;

    HeapArea heapArea_;
    LinearAllocator allocator_;
    LinearArena arena_;
    AllocationTable table_;
};

X_NAMESPACE_END

#endif // !X_ENABLE_MEMORY_DEBUG_POLICIES_DEFS

#endif // !X_EXTENDEDMEMORYTRACKING_H_
