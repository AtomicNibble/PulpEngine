
X_NAMESPACE_BEGIN(core)

template<class LockT>
X_INLINE UniqueLock<LockT>::UniqueLock() :
    pLock_(nullptr),
    owns_(false)
{
}

template<class LockT>
X_INLINE UniqueLock<LockT>::UniqueLock(LockT& lock) :
    pLock_(&lock),
    owns_(false)
{
}

template<class LockT>
X_INLINE UniqueLock<LockT>::UniqueLock(LockT& lock, adopt_lock_t) :
    pLock_(&lock),
    owns_(true)
{
}

template<class LockT>
X_INLINE UniqueLock<LockT>::UniqueLock(LockT& lock, try_to_lock_t) :
    pLock_(&lock),
    owns_(lock.TryEnter())
{
}

template<class LockT>
X_INLINE UniqueLock<LockT>::UniqueLock(UniqueLock&& oth) :
    pLock_(oth.pLock_),
    owns_(oth.owns_)
{
    oth.pLock_ = nullptr;
    oth.owns_ = false;
}

template<class LockT>
X_INLINE UniqueLock<LockT>& UniqueLock<LockT>::operator=(UniqueLock&& oth)
{
    if (this != &oth) {
        if (owns_) {
            pLock->Leave();
        }

        pLock_ = oth.pLock_;
        owns_ = oth.owns_;
        oth.pLock_ = nullptr;
        oth.owns_ = false;
    }

    return *this;
}

template<class LockT>
X_INLINE UniqueLock<LockT>::~UniqueLock()
{
    if (owns_) {
        pLock_->Leave();
    }
}

template<class LockT>
X_INLINE void UniqueLock<LockT>::Enter(void)
{
    validate();
    pLock_->Enter();
    owns_ = true;
}

template<class LockT>
X_INLINE bool UniqueLock<LockT>::TryEnter(void)
{
    validate();
    owns_ = pLock_->TryEnter();
    return owns_;
}

template<class LockT>
X_INLINE void UniqueLock<LockT>::Leave(void)
{
    X_ASSERT_NOT_NULL(pLock_);
    X_ASSERT(owns_, "Lock not owned")(pLock_, owns_);

    pLock_->Leave();
    owns_ = true;
}

template<class LockT>
X_INLINE bool UniqueLock<LockT>::ownsLock(void) const
{
    return owns_;
}

template<class LockT>
X_INLINE UniqueLock<LockT>::operator bool() const
{
    return owns_;
}

template<class LockT>
X_INLINE void UniqueLock<LockT>::swap(UniqueLock& oth)
{
    core::Swap(pLock_, oth.pLock_);
    core::Swap(owns_, oth.owns_);
}

template<class LockT>
void UniqueLock<LockT>::validate(void) const
{
    X_ASSERT_NOT_NULL(pLock_);
    X_ASSERT(!owns_, "Already own lock")(pLock_, owns_);
}

X_NAMESPACE_END
