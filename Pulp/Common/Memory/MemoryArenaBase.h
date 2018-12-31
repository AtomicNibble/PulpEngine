#pragma once

#ifndef X_MEMORYARENABASE_H_
#define X_MEMORYARENABASE_H_

#include "Memory/MemoryArenaStatistics.h"

#if X_COMPILER_CLANG
#include <vector>
#else
#include <Containers\FixedArray.h>
#endif // !X_COMPILER_CLANG

X_NAMESPACE_BEGIN(core)

class MemoryArenaBase
{
    static const int MAX_ARENA_CHILDREN = 20;

public:
#if X_COMPILER_CLANG
    typedef std::vector<MemoryArenaBase*> ArenaArr;
#else
    typedef core::FixedArray<MemoryArenaBase*, MAX_ARENA_CHILDREN> ArenaArr;
#endif // !X_COMPILER_CLANG

public:
    virtual ~MemoryArenaBase(void) = default;

    /// The ID of an allocation is a human-readable string that is used to identify an allocation.
    /// typeName is a human-readable string that denotes the type of an allocation.
    virtual void* allocate(size_t size, size_t alignment, size_t offset X_MEM_HUMAN_IDS_CB(const char* ID) X_MEM_HUMAN_IDS_CB(const char* typeName) X_SOURCE_INFO_MEM_CB(const SourceInfo& sourceInfo)) X_ABSTRACT;

    virtual void free(void* ptr) X_ABSTRACT;
    virtual void free(void* ptr, size_t size) X_ABSTRACT;

    virtual size_t getSize(void* ptr) X_ABSTRACT;

    /// Returns how much of the buffer you can use.
    virtual size_t usableSize(void* ptr) const X_ABSTRACT;

    /// Returns statistics regarding the allocations made by the memory arena.
    virtual MemoryArenaStatistics getStatistics(void) const X_ABSTRACT;

    virtual MemoryAllocatorStatistics getAllocatorStatistics(bool children = false) const X_ABSTRACT;

    virtual bool isThreadSafe(void) const X_ABSTRACT;

    inline const ArenaArr& getChildrenAreas(void) const
    {
        return children_;
    }

    // adds it baby.
    inline void addChildArena(MemoryArenaBase* arena)
    {
        if (children_.size() == MAX_ARENA_CHILDREN) {
            X_WARNING("Memory", "can't add child arena exceeded max: %i", MAX_ARENA_CHILDREN);
        }
        else {
            if (children_.find(arena) == ArenaArr::invalid_index) {
                children_.push_back(arena);
            }
        }
    }


protected:
    ArenaArr children_;
};

X_NAMESPACE_END

#endif // X_MEMORYARENABASE_H_
