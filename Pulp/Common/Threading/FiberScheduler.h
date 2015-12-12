#pragma once

#include "Util\ReferenceCounted.h"
#include "Util\ReferenceCountedOwner.h"

#include "Containers\Fifo.h"
#include "Containers\FixedFifo.h"
#include "Containers\Array.h"

#include "Traits\FunctionTraits.h"
#include "Threading\AtomicInt.h"
#include "Threading\Spinlock.h"
#include "Threading\Signal.h"
#include "Threading\ThreadLocalStorage.h"

#include "Fiber.h"

X_NAMESPACE_BEGIN(core)

namespace Fiber
{
	class Scheduler;

	typedef core::traits::Function<void (Scheduler*, void*)> TaskFunction;

	struct Task 
	{
		TaskFunction::Pointer Function;
		void* pArgData;
	};

	/*
	struct RefCountedAtomicInt : public core::AtomicInt, public ReferenceCounted<core::AtomicInt>
	{

	};
	*/

	typedef ReferenceCountedInstance<core::AtomicInt> RefCountedAtomicInt;
	typedef core::ReferenceCountedOwner<RefCountedAtomicInt> RefCountedAtomicIntOwner;

	struct TaskBundle
	{
		TaskBundle();
		TaskBundle(Task task, core::AtomicInt* counter);

		Task task;
		core::AtomicInt* pCounter;
	};

	struct WaitingTask
	{
		WaitingTask();

		FiberHandle fiber;
		core::AtomicInt* pCounter;
		int32_t val;
	};

	template<typename T>
	class ThreadQue
	{
	public:
		ThreadQue(core::MemoryArenaBase* arena, size_t size);
		~ThreadQue();

		void Add(const T item);
		void Add(T* pItems, size_t numItems);
		bool tryPop(T& item);

		// for batch adding
		void Lock(void);
		void Unlock(void);
		void Add_nolock(const T item);

		size_t numItems(void) const;

	protected:
		core::Spinlock lock_;
		core::Fifo<T> list_;
	};

	template<typename T>
	class ThreadQueBlocking : public ThreadQue<T>
	{
	public:
		using ThreadQue<T>::ThreadQue;


		void Add(const T item);
		void Add(T* pItems, size_t numItems);
		void Pop(T& item);

	private:
		core::Signal signal_;
	};

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

		if (list_.isEmpty())
			return false;

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
	size_t ThreadQue<T>::numItems(void) const
	{
		// any point locking for this?
		return list_.size();
	}


	class Scheduler
	{
		static const size_t FIBER_STACK_SIZE = 2048;

		static const uint32_t HW_THREAD_MAX = 32; // max even if hardware supports more.
		static const uint32_t HW_THREAD_NUM_DELTA = 1; // num = Min(max,hw_num-delta);
		static const uint32_t FIBER_POOL_SIZE = 64;

	public:
		Scheduler();
		~Scheduler();

		bool StartUp(void);
		void ShutDown(void);

		RefCountedAtomicIntOwner AddTask(Task task);
		RefCountedAtomicIntOwner AddTasks(Task* pTasks, size_t numTasks);

		void WaitForCounter(RefCountedAtomicIntOwner& counter, int32_t value);

	private:
		bool CreateFibers(void);
		bool CreateFibersForThread(uint32_t threadId);
		bool StartThreads(void);

	private:
		bool GetTask(TaskBundle& task);
		void SwitchFibers(FiberHandle fiberToSwitchTo);

		size_t GetCurrentThreadIndex(void) const;
		FiberHandle GetSwitchFiberForThread(void) const;
		FiberHandle GetWaitFiberForThread(void) const;

		static Thread::ReturnValue ThreadRun(const Thread& thread);

		static void __stdcall FiberStart(void* pArg);
		static void __stdcall FiberSwitchStart(void* pArg);
		static void __stdcall CounterWaitStart(void* pArg);

	private:
		typedef ThreadQue<TaskBundle> TaskQue;
		typedef core::Array<WaitingTask> WaitingTaskArr;
		typedef ThreadQueBlocking<FiberHandle> FiberPool;
		typedef core::Array<std::pair<uint32_t, size_t>> FiberIndexArr;
		typedef core::Array<FiberHandle> FiberArr;


		core::AtomicInt stop_;
		core::AtomicInt activeWorkers_;

		TaskQue tasks_;

		WaitingTaskArr waitingTasks_;
		Spinlock waitingTaskLock_;

		Thread threads_[HW_THREAD_MAX];
		FiberPool fibers_;

		// used to turn thread ID into fiber index.
		FiberIndexArr threadToFiberIndex_;
		
		FiberArr fiberSwitchingFibers_;
		FiberArr counterWaitingFibers_;


		Fiber::FiberHandle mainThreadFiber_;
		uint32_t startUpThreadId_;
		uint32_t numThreads_;
	};



} // namespadce Fiber


X_NAMESPACE_END