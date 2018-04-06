#pragma once

#include <Threading\ScopedLock.h>

X_NAMESPACE_BEGIN(core)

class SharedLock
{
public:
    typedef ScopedLock<SharedLock> ScopedLock;
    typedef ScopedLockShared<SharedLock> ScopedLockShared;

public:
    X_INLINE SharedLock();
    X_INLINE ~SharedLock();

    X_INLINE void Enter(void);
    X_INLINE bool TryEnter(void);
    X_INLINE void Leave(void);

    X_INLINE void EnterShared(void);
    X_INLINE bool TryEnterShared(void);
    X_INLINE void LeaveShared(void);

    X_INLINE SRWLOCK* GetNativeObject(void);

private:
    X_NO_COPY(SharedLock);
    X_NO_ASSIGN(SharedLock);

private:
    SRWLOCK smtx_;
};

X_NAMESPACE_END

#include "SharedLock.inl"