

X_INLINE Semaphore::Semaphore(int initialValue, int maximumValue)
{
	sema_ = CreateSemaphore(
		NULL,           // default security attributes
		initialValue,  // initial count
		maximumValue,  // maximum count
		NULL);          // unnamed semaphore
}

X_INLINE Semaphore::~Semaphore(void)
{
	::CloseHandle(sema_);
}

X_INLINE void Semaphore::Signal(void)
{
	ReleaseSemaphore(sema_, 1, NULL);
}

X_INLINE void Semaphore::Wait(void)
{
	WaitForSingleObject(sema_, INFINITE);
}

X_INLINE bool Semaphore::TryWait(void)
{
	return WaitForSingleObject(sema_, 0) == WAIT_OBJECT_0;
}

