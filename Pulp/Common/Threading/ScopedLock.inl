
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


X_NAMESPACE_END