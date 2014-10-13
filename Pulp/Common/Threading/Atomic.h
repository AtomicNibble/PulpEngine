#pragma once

#ifndef _X_THREADING_ATOMIC_H_
#define _X_THREADING_ATOMIC_H_

X_NAMESPACE_BEGIN(core)

namespace atomic
{
	/// \brief Atomically increments the integer at location \a memory, and returns the resulting incremented value.
	/// \remark \a memory must be aligned to a 32-bit boundary.
	X_INLINE int32_t Increment(volatile int32_t* memory);

	/// \brief Atomically decrements the integer at location \a memory, and returns the resulting decremented value.
	/// \remark \a memory must be aligned to a 32-bit boundary.
	X_INLINE int32_t Decrement(volatile int32_t* memory);

	/// \brief Atomically adds a value to the integer at location \a memory, and returns its initial value.
	/// \remark \a memory must be aligned to a 32-bit boundary.
	X_INLINE int32_t Add(volatile int32_t* memory, int32_t value);

	/// \brief Atomically exchanges the integer at location \a memory with the given value, and returns the integer's initial value.
	/// \remark \a memory must be aligned to a 32-bit boundary.
	X_INLINE int32_t Exchange(volatile int32_t* memory, int32_t value);

	/// \brief Atomically performs a comparison of the integer at location \a memory and the \a comperand value.
	/// \details If the value at \a memory is equal to the \a comperand, the \a exchange value is stored at \a memory.
	/// The return value is the initial value of the integer at location \a memory.
	/// \remark \a memory must be aligned to a 32-bit boundary.
	X_INLINE int32_t CompareExchange(volatile int32_t* memory, int32_t exchange, int32_t comperand);
}

#include "Atomic.inl"

X_NAMESPACE_END


#endif // !_X_THREADING_ATOMIC_H_

