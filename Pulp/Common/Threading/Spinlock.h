#pragma once


#ifndef _X_THREADING_SPINLOCK_H_
#define _X_THREADING_SPINLOCK_H_

#include "Atomic.h"
#include "Thread.h"
#include "ScopedLock.h"

X_NAMESPACE_BEGIN(core)

class Spinlock
{
public:
	typedef typename ScopedLock<Spinlock> ScopedLock;

public:
	X_INLINE Spinlock(void);

	X_INLINE void Enter(void);
	X_INLINE bool TryEnter(void);
	X_INLINE bool TryEnter(unsigned int tries);
	X_INLINE void Leave(void);

private:
	X_ALIGNED_SYMBOL(volatile int32_t locked_, 4);
};


class SpinlockRecursive
{
public:
	typedef typename ScopedLock<SpinlockRecursive> ScopedLock;

public:
	X_INLINE SpinlockRecursive(void);

	X_INLINE void Enter(void);
	X_INLINE void Leave(void);

private:
	X_ALIGNED_SYMBOL(volatile int32_t locked_, 4);
	uint32_t threadId_;
	uint32_t count_;
};


#include "Spinlock.inl"

X_NAMESPACE_END


#endif // !_X_THREADING_SPINLOCK_H_

