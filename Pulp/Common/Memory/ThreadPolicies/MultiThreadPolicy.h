#pragma once

#ifndef X_MULTITHREADPOLICY_H
#define X_MULTITHREADPOLICY_H

X_NAMESPACE_BEGIN(core)

/// \ingroup Memory
/// \brief A class that implements a threading policy for memory arenas.
/// \details This class implements the concepts of a threading policy as expected by the MemoryArena class. It is
/// intended to be used by multi-threaded arenas, and works with different synchronization primitives.
///
/// The concepts expected from the template class \c SynchronizationPrimitive are the following:
/// - It must implement a method \verbatim void Enter(void);\endverbatim that enters/locks the primitive.
/// - It must implement a method \verbatim void Leave(void);\endverbatim that leaves/unlocks the primitive.
///
/// As an example, a CriticalSection can be used as a synchronization primitive with the MultiThreadPolicy:
/// \code
///   // define a multi-threaded policy
///   typedef core::MultiThreadPolicy<core::CriticalSection> ThreadSafePolicy;
///
///   // defines a multi-threaded arena that uses a pool allocator, and simple bounds checking, tracking, and tagging
///   typedef core::MemoryArena<core::PoolAllocator, ThreadSafePolicy, core::SimpleBoundsChecking, core::SimpleMemoryTracking, core::SimpleMemoryTagging> PoolArena;
/// \endcode
/// \sa SingleThreadPolicy
template<class SynchronizationPrimitive>
class MultiThreadPolicy
{
public:
    /// A human-readable string literal containing the policy's type.
    static const char* const TYPE_NAME;
    static const bool IS_THREAD_SAFE = true;

    /// Enters the synchronization primitive.
    inline void Enter(void);

    /// Leaves the synchronization primitive.
    inline void Leave(void);

private:
    SynchronizationPrimitive primitive_;
};

#include "MultiThreadPolicy.inl"

X_NAMESPACE_END

#endif
