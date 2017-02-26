
X_NAMESPACE_BEGIN(core)

template<typename T, typename SynchronizationPrimitive>
ThreadQue<T, SynchronizationPrimitive>::ThreadQue(core::MemoryArenaBase* arena) : 
	que_(arena)
{

}

template<typename T, typename SynchronizationPrimitive>
ThreadQue<T, SynchronizationPrimitive>::ThreadQue(core::MemoryArenaBase* arena, size_t num) : 
	que_(arena, num)
{

}


template<typename T, typename SynchronizationPrimitive>
ThreadQue<T, SynchronizationPrimitive>::~ThreadQue()
{

}


template<typename T, typename SynchronizationPrimitive>
void ThreadQue<T, SynchronizationPrimitive>::reserve(size_t num)
{
	que_.reserve(num);
}

template<typename T, typename SynchronizationPrimitive>
void ThreadQue<T, SynchronizationPrimitive>::clear(void)
{
	SynchronizationPrimitive::ScopedLock lock(primitive_);

	que_.clear();
}



template<typename T, typename SynchronizationPrimitive>
void ThreadQue<T, SynchronizationPrimitive>::push(T const& value)
{
	SynchronizationPrimitive::ScopedLock lock(primitive_);

	que_.push(value);
}

template<typename T, typename SynchronizationPrimitive>
template<class UnaryPredicate>
bool ThreadQue<T, SynchronizationPrimitive>::push_unique_if(T const& value, UnaryPredicate p)
{
	SynchronizationPrimitive::ScopedLock lock(primitive_);

	if (!que_.contains_if(p)) {
		que_.push(value);
		return true;
	}

	return false;
}

template<typename T, typename SynchronizationPrimitive>
bool ThreadQue<T, SynchronizationPrimitive>::tryPop(T& value)
{
	SynchronizationPrimitive::ScopedLock lock(primitive_);

	if (que_.isEmpty())
		return false;

	value = que_.peek();
	que_.pop();
	return true;
}

template<typename T, typename SynchronizationPrimitive>
size_t ThreadQue<T, SynchronizationPrimitive>::size(void) const
{
	return que_.size();
}

template<typename T, typename SynchronizationPrimitive>
bool ThreadQue<T, SynchronizationPrimitive>::isEmpty(void) const
{
	return que_.isEmpty();
}

template<typename T, typename SynchronizationPrimitive>
bool ThreadQue<T, SynchronizationPrimitive>::isNotEmpty(void) const
{
	return !que_.isEmpty();
}


// --------------------------------------

template<typename T, typename SynchronizationPrimitive>
void ThreadQueBlocking<T, SynchronizationPrimitive>::push(T const& value)
{
	ThreadQue<T, SynchronizationPrimitive>::push(value);
	signal_.raise();
}

template<typename T, typename SynchronizationPrimitive>
void ThreadQueBlocking<T, SynchronizationPrimitive>::pop(T& value)
{
	X_DISABLE_WARNING(4127)
		while (true)
			X_ENABLE_WARNING(4127)
		{
			lock_.Enter();
			// if que empty wait
			if (list_.isEmpty())
			{
				lock_.Leave();
				signal_.wait();
				// loop around to reauire lock to check if still empty.
			}
			else
			{
				break; // break out, we still own lock
			}
		}

	value = que_.peek();
	que_.pop();

	// clear signal and unlock
	signal_.clear();
	lock_.Leave();
}

template<typename T, typename SynchronizationPrimitive>
T ThreadQueBlocking<T, SynchronizationPrimitive>::pop(void)
{
	X_DISABLE_WARNING(4127)
	while (true)
	X_ENABLE_WARNING(4127)
	{
		lock_.Enter();
			// if que empty wait
		if (list_.isEmpty())
		{
			lock_.Leave();
			signal_.wait();
			// loop around to reauire lock to check if still empty.
		}
		else
		{
			break; // break out, we still own lock
		}
	}

	T value = que_.peek();
	que_.pop();

	// clear signal and unlock
	signal_.clear();
	lock_.Leave();

	return value;
}

// --------------------------------------


template<typename T>
void ThreadQueBlocking<T, core::CriticalSection>::push(T const& value)
{
	ThreadQue<T, CriticalSection>::push(value);
	cond_.NotifyOne();
}


template<typename T>
void ThreadQueBlocking<T, core::CriticalSection>::pop(T& value)
{
	CriticalSection::ScopedLock lock(primitive_);

	while (que_.isEmpty())
	{
		cond_.Wait(primitive_);
	}

	value = que_.peek();
	que_.pop();
}


template<typename T>
T ThreadQueBlocking<T, core::CriticalSection>::pop(void)
{
	CriticalSection::ScopedLock lock(primitive_);

	while (que_.isEmpty())
	{
		cond_.Wait(primitive_);
	}

	T value = que_.peek();
	que_.pop();
	return value;
}


X_NAMESPACE_END
