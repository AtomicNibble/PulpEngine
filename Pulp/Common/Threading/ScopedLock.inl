
X_NAMESPACE_BEGIN(core)

template<class ThreadPolicy>
ScopedLock<ThreadPolicy>::ScopedLock(ThreadPolicy& policy) :
    policy_(policy)
{
    policy_.Enter();
}

template<class ThreadPolicy>
ScopedLock<ThreadPolicy>::~ScopedLock(void)
{
    policy_.Leave();
}

// -----------------------------------------------------

template<class ThreadPolicy>
ScopedLockShared<ThreadPolicy>::ScopedLockShared(ThreadPolicy& policy) :
    policy_(policy)
{
    policy_.EnterShared();
}

template<class ThreadPolicy>
ScopedLockShared<ThreadPolicy>::~ScopedLockShared(void)
{
    policy_.LeaveShared();
}

X_NAMESPACE_END