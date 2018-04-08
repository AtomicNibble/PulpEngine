#pragma once

#ifndef _X_THREADING_ATOMIC_H_
#define _X_THREADING_ATOMIC_H_

X_NAMESPACE_BEGIN(core)

namespace atomic
{
    /// \brief Atomically increments the integer at location \a memory, and returns the resulting incremented value.
    /// \remark \a memory must be aligned to a 32-bit boundary.
    template<typename T>
    X_INLINE T Increment(volatile T* memory);

    /// \brief Atomically decrements the integer at location \a memory, and returns the resulting decremented value.
    /// \remark \a memory must be aligned to a 32-bit boundary.
    template<typename T>
    X_INLINE T Decrement(volatile T* memory);

    /// \brief Atomically adds a value to the integer at location \a memory, and returns its initial value.
    /// \remark \a memory must be aligned to a 32-bit boundary.
    template<typename T>
    X_INLINE T Add(volatile T* memory, T value);

    template<typename T>
    X_INLINE T Subtract(volatile T* memory, T value);

    template<typename T>
    X_INLINE T And(volatile T* memory, T value);

    template<typename T>
    X_INLINE T Or(volatile T* memory, T value);

    /// \brief Atomically exchanges the integer at location \a memory with the given value, and returns the integer's initial value.
    /// \remark \a memory must be aligned to a 32-bit boundary.
    template<typename T>
    X_INLINE T Exchange(volatile T* memory, T value);

    /// \brief Atomically performs a comparison of the integer at location \a memory and the \a comperand value.
    /// \details If the value at \a memory is equal to the \a comperand, the \a exchange value is stored at \a memory.
    /// The return value is the initial value of the integer at location \a memory.
    /// \remark \a memory must be aligned to a 32-bit boundary.
    template<typename T>
    X_INLINE T CompareExchange(volatile T* memory, T exchange, T comperand);
} // namespace atomic


X_NAMESPACE_END

#include "Atomic.inl"

#endif // !_X_THREADING_ATOMIC_H_
