#pragma once


#ifndef _X_THREADING_SPINLOCK_H_
#define _X_THREADING_SPINLOCK_H_

#include "Atomic.h"

X_NAMESPACE_BEGIN(core)

class Spinlock
{
public:
	X_INLINE Spinlock(void);

	X_INLINE void Enter(void);
	X_INLINE bool TryEnter(void);
	X_INLINE bool TryEnter(unsigned int tries);
	X_INLINE void Leave(void);

	class ScopedLock
	{
	public:
		X_INLINE explicit ScopedLock(Spinlock& spinlock);
		X_INLINE ~ScopedLock(void);

	private:
		X_NO_COPY(ScopedLock);
		X_NO_ASSIGN(ScopedLock);

		Spinlock& spinlock_;
	};

private:
	X_ALIGNED_SYMBOL(volatile int32_t locked_, 4);
};

#include "Spinlock.inl"

X_NAMESPACE_END


#endif // !_X_THREADING_SPINLOCK_H_

