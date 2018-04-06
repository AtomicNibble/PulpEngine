
X_NAMESPACE_BEGIN(core)

template<typename T, size_t N, typename SynchronizationPrimitive>
void FixedThreadQueBase<T, N, SynchronizationPrimitive>::clear(void)
{
    SynchronizationPrimitive::ScopedLock lock(primitive_);
    que_.clear();
}

template<typename T, size_t N, typename SynchronizationPrimitive>
void FixedThreadQueBase<T, N, SynchronizationPrimitive>::free(void)
{
    SynchronizationPrimitive::ScopedLock lock(primitive_);
    que_.free();
}

template<typename T, size_t N, typename SynchronizationPrimitive>
size_t FixedThreadQueBase<T, N, SynchronizationPrimitive>::size(void)
{
    SynchronizationPrimitive::ScopedLock lock(primitive_);
    return que_.size();
}

template<typename T, size_t N, typename SynchronizationPrimitive>
size_t FixedThreadQueBase<T, N, SynchronizationPrimitive>::freeSpace(void)
{
    SynchronizationPrimitive::ScopedLock lock(primitive_);
    return que_.size();
}

template<typename T, size_t N, typename SynchronizationPrimitive>
bool FixedThreadQueBase<T, N, SynchronizationPrimitive>::isEmpty(void) const
{
    return que_.isEmpty();
}

template<typename T, size_t N, typename SynchronizationPrimitive>
bool FixedThreadQueBase<T, N, SynchronizationPrimitive>::isNotEmpty(void) const
{
    return que_.isNotEmpty();
}

#if 0
// --------------------------------------



// --------------------------------------
#endif

template<typename T, size_t N>
void FixedThreadQue<T, N, core::CriticalSection>::push(T const& value)
{
    {
        CriticalSection::ScopedLock lock(primitive_);

        while (que_.freeSpace() == 0) {
            postPopCond_.Wait(primitive_);
        }

        que_.push(value);
    }

    cond_.NotifyOne();
}

template<typename T, size_t N>
void FixedThreadQue<T, N, core::CriticalSection>::push(T&& value)
{
    {
        CriticalSection::ScopedLock lock(primitive_);

        while (que_.freeSpace() == 0) {
            postPopCond_.Wait(primitive_);
        }

        que_.push(std::forward<T>(value));
    }

    cond_.NotifyOne();
}

template<typename T, size_t N>
bool FixedThreadQue<T, N, core::CriticalSection>::tryPop(T& value)
{
    CriticalSection::ScopedLock lock(primitive_);

    if (que_.isEmpty()) {
        return false;
    }

    value = std::move(que_.peek());
    que_.pop();

    postPopCond_.NotifyOne();
    return true;
}

template<typename T, size_t N>
void FixedThreadQue<T, N, core::CriticalSection>::pop(T& value)
{
    CriticalSection::ScopedLock lock(primitive_);

    while (que_.isEmpty()) {
        cond_.Wait(primitive_);
    }

    value = std::move(que_.peek());
    que_.pop();

    postPopCond_.NotifyOne();
}

template<typename T, size_t N>
T FixedThreadQue<T, N, core::CriticalSection>::pop(void)
{
    CriticalSection::ScopedLock lock(primitive_);

    while (que_.isEmpty()) {
        cond_.Wait(primitive_);
    }

    T value = std::move(que_.peek());
    que_.pop();

    postPopCond_.NotifyOne();

    return value;
}

X_NAMESPACE_END
