#pragma once

#ifndef X_SINGLETHREADPOLICY_H
#define X_SINGLETHREADPOLICY_H

X_NAMESPACE_BEGIN(core)

/// \ingroup Memory
/// \brief A class that implements a threading policy for memory arenas.
/// \details This class implements the concepts of a threading policy as expected by the MemoryArena class. It is
/// intended to be used by single-threaded arenas, and only contains empty implementations.
/// \sa MultiThreadPolicy
class SingleThreadPolicy
{
public:
    /// A human-readable string literal containing the policy's type.
    static const char* const TYPE_NAME;
    static const bool IS_THREAD_SAFE = false;

    /// Empty implementation.
    inline void Enter(void) const
    {
    }

    /// Empty implementation.
    inline void Leave(void) const
    {
    }
};

X_NAMESPACE_END

#endif
