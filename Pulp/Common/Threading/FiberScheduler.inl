#pragma once

X_NAMESPACE_BEGIN(core)

namespace Fiber
{

	template<typename T>
	ThreadQue<T>::ThreadQue() :
		list_(nullptr)
	{

	}


	template<typename T>
	ThreadQue<T>::ThreadQue(core::MemoryArenaBase* arena, size_t size) :
		list_(arena, size)
	{

	}

	template<typename T>
	ThreadQue<T>::~ThreadQue()
	{

	}

	template<typename T>
	void ThreadQue<T>::setArena(core::MemoryArenaBase* arena, size_t size)
	{
		list_.setArena(arena);
		list_.reserve(size);
	}

	template<typename T>
	void ThreadQue<T>::setArena(core::MemoryArenaBase* arena)
	{
		list_.setArena(arena);
	}

	template<typename T>
	void ThreadQue<T>::Add(const T item)
	{
		core::Spinlock::ScopedLock lock(lock_);

		list_.push(item);
	}

	template<typename T>
	void ThreadQue<T>::Add(T* pItems, size_t numItems)
	{
		core::Spinlock::ScopedLock lock(lock_);

		for (size_t i = 0; i < numItems; i++) {
			list_.push(pItems[i]);
		}
	}


	template<typename T>
	bool ThreadQue<T>::tryPop(T& item)
	{
		core::Spinlock::ScopedLock lock(lock_);

		if (list_.isEmpty()) {
			return false;
		}

		item = list_.peek();
		list_.pop();
		return true;
	}

	template<typename T>
	void ThreadQue<T>::Lock(void)
	{
		lock_.Enter();
	}

	template<typename T>
	void ThreadQue<T>::Unlock(void)
	{
		lock_.Leave();
	}

	template<typename T>
	void ThreadQue<T>::Add_nolock(const T item)
	{
		// should fail to enter
		//	X_ASSERT(!lock_.TryEnter(), "Add_nolock called without lock ownership")();

		list_.push(item);
	}

	template<typename T>
	size_t ThreadQue<T>::numItems(void)
	{
		core::Spinlock::ScopedLock lock(lock_);

		return list_.size();
	}

	template<typename T>
	bool ThreadQue<T>::isNotEmpty(void) const
	{
		return list_.isNotEmpty();
	}

	// =============================

	template<typename T>
	void ThreadQueBlocking<T>::Add(const T item)
	{
		ThreadQue<T>::Add(item);
		signal_.raise();
	}

	template<typename T>
	void ThreadQueBlocking<T>::Add(T* pItems, size_t numItems)
	{
		ThreadQue<T>::Add(pItems, numItems);
		signal_.raise();
	}

	template<typename T>
	void ThreadQueBlocking<T>::Pop(T& item)
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

		item = list_.peek();
		list_.pop();

		// clear signal and unlock
		signal_.clear();
		lock_.Leave();
	}



} // namespace Fiber

X_NAMESPACE_END