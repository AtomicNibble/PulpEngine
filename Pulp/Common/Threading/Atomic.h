#pragma once

#ifndef _X_THREADING_ATOMIC_H_
#define _X_THREADING_ATOMIC_H_

X_NAMESPACE_BEGIN(core)

namespace atomic
{
    // Atomically increments the integer at location memory, and returns the resulting incremented value.
    // memory must be aligned to a 32-bit boundary.
    template<typename T>
    X_INLINE T Increment(volatile T* memory);

    // Atomically decrements the integer at location memory, and returns the resulting decremented value.
    // memory must be aligned to a 32-bit boundary.
    template<typename T>
    X_INLINE T Decrement(volatile T* memory);

    // Atomically adds a value to the integer at location memory, and returns its initial value.
    // memory must be aligned to a 32-bit boundary.
    template<typename T>
    X_INLINE T Add(volatile T* memory, T value);

    template<typename T>
    X_INLINE T Subtract(volatile T* memory, T value);

    template<typename T>
    X_INLINE T And(volatile T* memory, T value);

    template<typename T>
    X_INLINE T Or(volatile T* memory, T value);

    // Atomically exchanges the integer at location memory with the given value, and returns the integer's initial value.
    // memory must be aligned to a 32-bit boundary.
    template<typename T>
    X_INLINE T Exchange(volatile T* memory, T value);

    // Atomically performs a comparison of the integer at location memory and the comperand value.
    // details If the value at memory is equal to the comperand, the exchange value is stored at memory.
    // The return value is the initial value of the integer at location memory.
    // memory must be aligned to a 32-bit boundary.
    template<typename T>
    X_INLINE T CompareExchange(volatile T* memory, T exchange, T comperand);
} // namespace atomic


X_NAMESPACE_END

#include "Atomic.inl"

#endif // _X_THREADING_ATOMIC_H_
