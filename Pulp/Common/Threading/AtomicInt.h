#pragma once

#ifndef _X_THREADING_ATOMIC_INT_H_
#define _X_THREADING_ATOMIC_INT_H_

#include <Threading\Atomic.h>

X_NAMESPACE_BEGIN(core)

class AtomicInt
{
public:
	/// Constructs an AtomicInt from an integer.
	X_INLINE explicit AtomicInt(int32_t value);

	X_INLINE int32_t operator=(int32_t value);
	X_INLINE int32_t operator+=(int32_t value);
	X_INLINE int32_t operator++(void);
	X_INLINE int32_t operator--(void);
	X_INLINE operator int32_t(void) const volatile;

private:
	X_NO_COPY(AtomicInt);
	X_NO_ASSIGN(AtomicInt);

	X_ALIGNED_SYMBOL(volatile int32_t value_, 4);
};

#include "AtomicInt.inl"

X_NAMESPACE_END

#endif // !_X_THREADING_ATOMIC_INT_H_
