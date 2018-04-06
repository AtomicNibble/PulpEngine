#pragma once

#ifndef X_SIMPLEMEMORYTAGGING_H_
#define X_SIMPLEMEMORYTAGGING_H_

#if X_ENABLE_MEMORY_DEBUG_POLICIES_DEFS

X_NAMESPACE_BEGIN(core)

/// \ingroup Memory
/// \brief A class that implements a tagging policy for memory arenas.
/// \details This class implements the concepts of a tagging policy as expected by the MemoryArena class. It fills
/// allocated memory with \c 0xCD, and freed memory with \c 0xDD. This makes it easier to recognize programming errors
/// such as dangling pointers and double deletes.
/// \sa NoMemoryTagging
class SimpleMemoryTagging
{
public:
    /// A human-readable string literal containing the policy's type.
    static const char* const TYPE_NAME;

    /// Tags the memory region with TAG_ALLOCATED.
    void TagAllocation(void* memory, size_t size);

    /// Tags the memory region with TAG_FREED.
    void TagDeallocation(void* memory, size_t size);

private:
    /// Defines the bit pattern which is used to tag an allocated memory region.
    static const int TAG_ALLOCATED = 0xBB;

    /// Defines the bit pattern which is used to tag a freed memory region.
    static const int TAG_FREED = 0xEE;
};

X_NAMESPACE_END

#endif // !X_ENABLE_MEMORY_DEBUG_POLICIES_DEFS

#endif // !X_SIMPLEMEMORYTAGGING_H_
