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
		Task();
		Task(TaskFunction::Pointer Function, void* pArgData);

		TaskFunction::Pointer Function;
		void* pArgData;
	};

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



} // namespace Fiber

X_NAMESPACE_END

#include "FiberScheduler.inl"