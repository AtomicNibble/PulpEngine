#pragma once

#ifndef X_NOMEMORYTAGGING_H
#define X_NOMEMORYTAGGING_H

X_NAMESPACE_BEGIN(core)

/// \ingroup Memory
/// \brief A class that implements a tagging policy for memory arenas.
/// \details This class implements the concepts of a tagging policy as expected by the MemoryArena class. It is
/// a no-op class, only containing empty implementations.
/// \sa SimpleMemoryTagging
class NoMemoryTagging
{
public:
    /// A human-readable string literal containing the policy's type.
    static const char* const TYPE_NAME;

    /// Empty implementation.
    inline void TagAllocation(void*, size_t) const
    {
    }

    /// Empty implementation.
    inline void TagDeallocation(void*, size_t) const
    {
    }
};

X_NAMESPACE_END

#endif
