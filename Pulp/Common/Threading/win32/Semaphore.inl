

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

X_INLINE void Semaphore::ReleaseSlot(void)
{
	ReleaseSemaphore(sema_, 1, NULL);
}

X_INLINE void Semaphore::AcquireSlot(void)
{
	WaitForSingleObject(sema_, INFINITE);
}

X_INLINE bool Semaphore::TryAcquireSlot(void)
{
	return WaitForSingleObject(sema_, 0) == WAIT_OBJECT_0;
}

