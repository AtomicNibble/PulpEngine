

X_NAMESPACE_BEGIN(core)

X_INLINE uint32_t Thread::GetID(void) const
{
	return ::GetThreadId(handle_);
}


X_INLINE void Thread::setData(void* pData)
{
	pData_ = pData;
}

X_INLINE void* Thread::getData(void) const
{
	return pData_;
}

X_INLINE void Thread::Sleep(uint32_t milliSeconds)
{
	::Sleep(milliSeconds);
}


X_INLINE void Thread::Yield(void)
{
	// http://msdn.microsoft.com/en-us/library/windows/desktop/ms686352(v=vs.85).aspx
	::SwitchToThread();
}

X_INLINE void Thread::YieldProcessor(void)
{
	// about a 9 cycle delay
#if !defined(__midl) && !defined(GENUTIL) && !defined(_GENIA64_) && defined(_IA64_)
	__yield();
#else
	_mm_pause();
#endif
}

X_INLINE uint32_t Thread::GetCurrentID(void)
{
	return ::GetCurrentThreadId();
}


// ----------------------------------------

template<class T>
X_INLINE void ThreadMember<T>::Create(const char* pName, uint32_t stackSize)
{
	handle_ = createThreadInternal(stackSize, (LPTHREAD_START_ROUTINE)ThreadFunctionDel_);

	name_.set(pName);

	if (handle_ == NULL)
	{
		lastError::Description Dsc;
		X_ERROR("Thread", "failed to create thread. Erorr: %s", lastError::ToString(Dsc));
	}
	else {
		state_ = State::READY;
	}
}


template<class T>
X_INLINE void ThreadMember<T>::Start(FunctionDelagate delagate)
{
	delagate_ = delagate;
	state_ = State::RUNNING;

	if (ResumeThread(handle_) == (DWORD)-1)
	{
		lastError::Description Dsc;
		X_ERROR("Thread", "failed to start thread. Erorr: %s", lastError::ToString(Dsc));
	}

	Thread::SetName(GetID(), name_.c_str());
}


template<class T>
X_INLINE void ThreadMember<T>::Stop(void)
{
	Thread::Stop();
}

template<class T>
X_INLINE void ThreadMember<T>::Join(void)
{
	Thread::Join();
}

template<class T>
X_INLINE bool ThreadMember<T>::ShouldRun(void) const volatile
{
	return Thread::ShouldRun();
}

template<class T>
X_INLINE bool ThreadMember<T>::HasFinished(void) const volatile
{
	return Thread::HasFinished();
}

template<class T>
X_INLINE bool ThreadMember<T>::SetThreadAffinity(const AffinityFlags flags)
{
	return Thread::SetThreadAffinity(flags);
}

template<class T>
X_INLINE void ThreadMember<T>::SetFPE(FPE::Enum fpe)
{
	Thread::SetFPE(fpe);
}

template<class T>
X_INLINE uint32_t ThreadMember<T>::GetID(void) const
{
	return Thread::GetID();
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