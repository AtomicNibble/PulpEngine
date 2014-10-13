#pragma once

#ifndef _X_THREADING_SEMAPHORE_H_
#define _X_THREADING_SEMAPHORE_H_

X_NAMESPACE_BEGIN(core)

class Semaphore
{
public:
	X_INLINE Semaphore(int initialValue, int maximumValue);
	X_INLINE ~Semaphore(void);

	X_INLINE void Signal(void);
	X_INLINE void Wait(void);
	X_INLINE bool TryWait(void);

private:
	HANDLE sema_;
};

#include "Semaphore.inl"

X_NAMESPACE_END


#endif // !_X_THREADING_SEMAPHORE_H_
