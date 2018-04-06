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

template<class ThreadPolicy>
class ScopedLockShared
{
public:
    inline explicit ScopedLockShared(ThreadPolicy& policy);
    inline ~ScopedLockShared(void);

private:
    X_NO_COPY(ScopedLockShared);
    X_NO_ASSIGN(ScopedLockShared);

    ThreadPolicy& policy_;
};

X_NAMESPACE_END

#include "ScopedLock.inl"
