#pragma once

#ifndef X_NOBOUNDSCHECKING_H_
#define X_NOBOUNDSCHECKING_H_

X_NAMESPACE_BEGIN(core)

/// \ingroup Memory
/// \brief A class that implements a bounds checking policy for memory arenas.
/// \details This class implements the concepts of a bounds checking policy as expected by the MemoryArena class. It is
/// a no-op class, only containing empty implementations.
/// \sa SimpleBoundsChecking
class NoBoundsChecking
{
public:
    /// A human-readable string literal containing the policy's type.
    static const char* const TYPE_NAME;

    /// Defines the number of guard bytes at the front of an allocation.
    static const size_t SIZE_FRONT = 0;

    /// Defines the number of guard bytes at the back of an allocation.
    static const size_t SIZE_BACK = 0;

    /// Empty implementation.
    inline void GuardFront(void*) const
    {
    }

    /// Empty implementation.
    inline void GuardBack(void*) const
    {
    }

    /// Empty implementation.
    inline void CheckFront(const void*) const
    {
    }

    /// Empty implementation.
    inline void CheckBack(const void*) const
    {
    }
};

X_NAMESPACE_END

#endif
