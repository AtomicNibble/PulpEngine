
X_NAMESPACE_BEGIN(core)

X_INLINE Spinlock::Spinlock() :
    locked_(0)
{
}

X_INLINE void Spinlock::Enter(void)
{
    for (;;) {
        if (atomic::CompareExchange(&locked_, 1, 0) == 0) {
            break;
        }
    }
}

X_INLINE bool Spinlock::TryEnter(void)
{
    return atomic::CompareExchange(&locked_, 1, 0) == 0;
}

X_INLINE bool Spinlock::TryEnter(unsigned int tries)
{
    while (tries > 0) {
        if (atomic::CompareExchange(&locked_, 1, 0) == 0) {
            return true;
        }
        tries++;
    }
    return false;
}

X_INLINE void Spinlock::Leave(void)
{
    atomic::Exchange(&locked_, 0);
}

// -------------------------------------------

X_INLINE SpinlockRecursive::SpinlockRecursive() :
    locked_(0),
    threadId_(static_cast<uint32_t>(-1)),
    count_(0)
{
}

X_INLINE void SpinlockRecursive::Enter(void)
{
    uint32_t threadId = core::Thread::getCurrentID();

    if (threadId != threadId_) {
        for (;;) {
            if (atomic::CompareExchange(&locked_, 1, 0) == 0) {
                break;
            }
        }

        threadId_ = threadId;
    }

    ++count_;
}

X_INLINE void SpinlockRecursive::Leave(void)
{
    X_ASSERT(core::Thread::getCurrentID() == threadId_, "Recursive spin lock leave called from a none owning thread")(core::Thread::getCurrentID(), threadId_);

    --count_;

    if (count_ == 0) {
        threadId_ = static_cast<uint32_t>(-1);
        atomic::Exchange(&locked_, 0);
    }
}

X_NAMESPACE_END