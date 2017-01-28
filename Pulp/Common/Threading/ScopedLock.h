#pragma once


X_NAMESPACE_BEGIN(core)


template<class ThreadPolicy>
class ScopedLock
{
public:
	inline explicit ScopedLock(ThreadPolicy& policy);
	inline ~ScopedLock(void);

private:
	X_NO_COPY(ScopedLock);
	X_NO_ASSIGN(ScopedLock);

	ThreadPolicy& policy_;
};


X_NAMESPACE_END

#include "ScopedLock.inl"
