

X_NAMESPACE_BEGIN(core)

X_INLINE uint32_t Thread::getID(void) const
{
    return ::GetThreadId(handle_);
}

X_INLINE Thread::State::Enum Thread::getState(void) const
{
    return state_;
}

X_INLINE void Thread::setData(void* pData)
{
    pData_ = pData;
}

X_INLINE void* Thread::getData(void) const
{
    return pData_;
}

X_INLINE void Thread::sleep(uint32_t milliSeconds)
{
    ::Sleep(milliSeconds);
}

X_INLINE void Thread::yield(void)
{
    // http://msdn.microsoft.com/en-us/library/windows/desktop/ms686352(v=vs.85).aspx
    ::SwitchToThread();
}

X_INLINE void Thread::yieldProcessor(void)
{
    // about a 9 cycle delay
#if !defined(__midl) && !defined(GENUTIL) && !defined(_GENIA64_) && defined(_IA64_)
    __yield();
#else
    _mm_pause();
#endif
}

X_INLINE void Thread::backOff(int32_t backoff)
{
    if (backoff < 10 && backoff > 0) {
        Thread::yieldProcessor();
    }
    else if (backoff < 20) {
        for (size_t i = 0; i != 50; i += 1) {
            Thread::yieldProcessor();
        }
    }
    else if (backoff < 28) {
        Thread::yield();
    }
    else if (backoff < 10) {
        Thread::sleep(0);
    }

    // rip
    Thread::sleep(1);
}

X_INLINE uint32_t Thread::getCurrentID(void)
{
    return ::GetCurrentThreadId();
}

// ----------------------------------------

template<class T>
X_INLINE void ThreadMember<T>::create(const char* pName, uint32_t stackSize)
{
    name_.set(pName);

    if (!createThreadInternal(stackSize, (LPTHREAD_START_ROUTINE)ThreadFunctionDel_)) {
        lastError::Description Dsc;
        X_ERROR("Thread", "failed to create thread. Erorr: %s", lastError::ToString(Dsc));
    }
    else {
        state_ = State::READY;
    }
}

template<class T>
X_INLINE void ThreadMember<T>::start(FunctionDelagate delagate)
{
    delagate_ = delagate;
    state_ = State::RUNNING;

    if (::ResumeThread(handle_) == (DWORD)-1) {
        lastError::Description Dsc;
        X_ERROR("Thread", "failed to start thread. Erorr: %s", lastError::ToString(Dsc));
    }

    Thread::setName(getID(), name_.c_str());
}

template<class T>
X_INLINE void ThreadMember<T>::stop(void)
{
    Thread::stop();
}

template<class T>
X_INLINE void ThreadMember<T>::join(void)
{
    Thread::join();
}

template<class T>
X_INLINE bool ThreadMember<T>::shouldRun(void) const volatile
{
    return Thread::shouldRun();
}

template<class T>
X_INLINE bool ThreadMember<T>::hasFinished(void) const volatile
{
    return Thread::hasFinished();
}

template<class T>
X_INLINE bool ThreadMember<T>::setThreadAffinity(const AffinityFlags flags)
{
    return Thread::setThreadAffinity(flags);
}

template<class T>
X_INLINE void ThreadMember<T>::setFPE(FPE::Enum fpe)
{
    Thread::setFPE(fpe);
}

template<class T>
X_INLINE uint32_t ThreadMember<T>::getID(void) const
{
    return Thread::getID();
}

template<class T>
X_INLINE Thread::State::Enum ThreadMember<T>::getState(void) const
{
    return Thread::getState();
}

template<class T>
X_INLINE uint32_t __stdcall ThreadMember<T>::ThreadFunctionDel_(void* threadInstance)
{
    ThreadMember<T>* pThis = reinterpret_cast<ThreadMember<T>*>(threadInstance);
    X_ASSERT_NOT_NULL(pThis);

    uint32_t ret = pThis->delagate_.Invoke(*pThis);
    pThis->state_ = State::FINISHED;
    return ret;
}

template<class T>
X_INLINE void ThreadMember<T>::setData(void* pData)
{
    Thread::setData(pData);
}

template<class T>
X_INLINE void* ThreadMember<T>::getData(void) const
{
    return Thread::getData();
}

X_NAMESPACE_END