
X_NAMESPACE_BEGIN(core)

template<typename QueT, typename SynchronizationPrimitive>
ThreadQueBase<QueT, SynchronizationPrimitive>::ThreadQueBase(core::MemoryArenaBase* arena) :
    que_(arena)
{
}

template<typename QueT, typename SynchronizationPrimitive>
ThreadQueBase<QueT, SynchronizationPrimitive>::ThreadQueBase(core::MemoryArenaBase* arena, size_type num) :
    que_(arena, num)
{
}

template<typename QueT, typename SynchronizationPrimitive>
ThreadQueBase<QueT, SynchronizationPrimitive>::~ThreadQueBase()
{
}

template<typename QueT, typename SynchronizationPrimitive>
void ThreadQueBase<QueT, SynchronizationPrimitive>::reserve(size_type num)
{
    typename SynchronizationPrimitive::ScopedLock lock(primitive_);

    que_.reserve(num);
}

template<typename QueT, typename SynchronizationPrimitive>
void ThreadQueBase<QueT, SynchronizationPrimitive>::clear(void)
{
    typename SynchronizationPrimitive::ScopedLock lock(primitive_);

    que_.clear();
}

template<typename QueT, typename SynchronizationPrimitive>
void ThreadQueBase<QueT, SynchronizationPrimitive>::free(void)
{
    typename SynchronizationPrimitive::ScopedLock lock(primitive_);

    que_.free();
}

template<typename QueT, typename SynchronizationPrimitive>
void ThreadQueBase<QueT, SynchronizationPrimitive>::push(Type const& value)
{
    typename SynchronizationPrimitive::ScopedLock lock(primitive_);

    que_.push(value);
}

template<typename QueT, typename SynchronizationPrimitive>
void ThreadQueBase<QueT, SynchronizationPrimitive>::push(Type&& value)
{
    typename SynchronizationPrimitive::ScopedLock lock(primitive_);

    que_.push(std::forward<Type>(value));
}

template<typename QueT, typename SynchronizationPrimitive>
template<class UnaryPredicate>
bool ThreadQueBase<QueT, SynchronizationPrimitive>::push_unique_if(Type const& value, UnaryPredicate p)
{
    typename SynchronizationPrimitive::ScopedLock lock(primitive_);

    if (!que_.contains_if(p)) {
        que_.push(value);
        return true;
    }

    return false;
}

template<typename QueT, typename SynchronizationPrimitive>
bool ThreadQueBase<QueT, SynchronizationPrimitive>::tryPop(Type& value)
{
    typename SynchronizationPrimitive::ScopedLock lock(primitive_);

    if (que_.isEmpty()) {
        return false;
    }

    value = std::move(que_.peek());
    que_.pop();
    return true;
}

template<typename QueT, typename SynchronizationPrimitive>
template<class CallBack>
bool ThreadQueBase<QueT, SynchronizationPrimitive>::tryPopAll(CallBack func)
{
    typename SynchronizationPrimitive::ScopedLock lock(primitive_);

    if (que_.isEmpty()) {
        return false;
    }

    while (!que_.isEmpty()) {
        func(que_.peek());
        que_.pop();
    }
    return true;
}

template<typename QueT, typename SynchronizationPrimitive>
size_t ThreadQueBase<QueT, SynchronizationPrimitive>::size(void)
{
    typename SynchronizationPrimitive::ScopedLock lock(primitive_);

    return que_.size();
}

template<typename QueT, typename SynchronizationPrimitive>
bool ThreadQueBase<QueT, SynchronizationPrimitive>::isEmpty(void) const
{
    return que_.isEmpty();
}

template<typename QueT, typename SynchronizationPrimitive>
bool ThreadQueBase<QueT, SynchronizationPrimitive>::isNotEmpty(void) const
{
    return que_.isNotEmpty();
}

// --------------------------------------

template<typename QueT, typename SynchronizationPrimitive>
void ThreadQueBlockingBase<QueT, SynchronizationPrimitive>::push(Type const& value)
{
    BaseT::push(value);
    signal_.raise();
}

template<typename QueT, typename SynchronizationPrimitive>
void ThreadQueBlockingBase<QueT, SynchronizationPrimitive>::push(Type&& value)
{
    BaseT::push(std::forward<Type>(value));
    signal_.raise();
}

template<typename QueT, typename SynchronizationPrimitive>
void ThreadQueBlockingBase<QueT, SynchronizationPrimitive>::pop(Type& value)
{
X_DISABLE_WARNING(4127)
    while (true)
    {
        BaseT::primitive_.Enter();
        // if que empty wait
        if (BaseT::que_.isEmpty()) {
            BaseT::primitive_.Leave();
            signal_.wait();
            // loop around to reauire lock to check if still empty.
        }
        else {
            break; // break out, we still own lock
        }
    }
X_ENABLE_WARNING(4127)

    value = std::move(BaseT::que_.peek());
    BaseT::que_.pop();

    // clear signal and unlock
    signal_.clear();
    BaseT::primitive_.Leave();
}

template<typename QueT, typename SynchronizationPrimitive>
typename ThreadQueBlockingBase<QueT, SynchronizationPrimitive>::Type ThreadQueBlockingBase<QueT, SynchronizationPrimitive>::pop(void)
{
X_DISABLE_WARNING(4127)
    while (true)
    {
        BaseT::primitive_.Enter();
        // if que empty wait
        if (BaseT::que_.isEmpty()) {
            BaseT::primitive_.Leave();
            signal_.wait();
            // loop around to reauire lock to check if still empty.
        }
        else {
            break; // break out, we still own lock
        }
    }
X_ENABLE_WARNING(4127)

    Type value = std::move(BaseT::que_.peek());
    BaseT::que_.pop();

    // clear signal and unlock
    signal_.clear();
    BaseT::primitive_.Leave();

    return value;
}

// --------------------------------------

template<typename QueT>
void ThreadQueBlockingBase<QueT, core::CriticalSection>::push(Type const& value)
{
    BaseT::push(value);
    cond_.NotifyOne();
}

template<typename QueT>
void ThreadQueBlockingBase<QueT, core::CriticalSection>::push(Type&& value)
{
    BaseT::push(std::forward<T>(value));
    cond_.NotifyOne();
}

template<typename QueT>
void ThreadQueBlockingBase<QueT, core::CriticalSection>::pop(Type& value)
{
    CriticalSection::ScopedLock lock(BaseT::primitive_);

    while (BaseT::que_.isEmpty()) {
        cond_.Wait(BaseT::primitive_);
    }

    value = std::move(BaseT::que_.peek());
    BaseT::que_.pop();
}

template<typename QueT>
typename ThreadQueBlockingBase<QueT, core::CriticalSection>::Type ThreadQueBlockingBase<QueT, core::CriticalSection>::pop(void)
{
    CriticalSection::ScopedLock lock(BaseT::primitive_);

    while (BaseT::que_.isEmpty()) {
        cond_.Wait(BaseT::primitive_);
    }

    Type value = std::move(BaseT::que_.peek());
    BaseT::que_.pop();
    return value;
}

X_NAMESPACE_END
