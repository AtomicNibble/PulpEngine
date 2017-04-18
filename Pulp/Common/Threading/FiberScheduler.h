#pragma once

#include "Containers\Fifo.h"
#include "Containers\Array.h"

#include "Traits\FunctionTraits.h"
#include "Threading\AtomicInt.h"
#include "Threading\Spinlock.h"
#include "Threading\Signal.h"
#include "Threading\ThreadLocalStorage.h"

#include <Memory\ThreadPolicies\MultiThreadPolicy.h>
#include <Memory\AllocationPolicies\PoolAllocator.h>
#include <Memory\HeapArea.h>


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
		ThreadQue();
		ThreadQue(core::MemoryArenaBase* arena, size_t size);
		~ThreadQue();

		void setArena(core::MemoryArenaBase* arena, size_t size);
		void setArena(core::MemoryArenaBase* arena);


		void Add(const T item);
		void Add(T* pItems, size_t numItems);
		bool tryPop(T& item);

		// for batch adding
		void Lock(void);
		void Unlock(void);
		void Add_nolock(const T item);

		bool isNotEmpty(void) const;
		size_t numItems(void);

	protected:
		core::Spinlock lock_;
		core::Fifo<T> list_;
	};

	template<typename T>
	class ThreadQueBlocking : public ThreadQue<T>
	{
	public:
		ThreadQueBlocking();
		ThreadQueBlocking(core::MemoryArenaBase* arena, size_t size);

		void Add(const T item);
		void Add(T* pItems, size_t numItems);
		void Pop(T& item);

	private:
		core::Signal signal_;
	};

	X_DECLARE_ENUM(JobPriority)(HIGH, NORMAL, NONE);

	X_DISABLE_WARNING(4324)

	class Scheduler
	{
		static const uint32_t HW_THREAD_MAX = 32; // max even if hardware supports more.
		static const uint32_t HW_THREAD_NUM_DELTA = 1; // num = Min(max,hw_num-delta);
		static const uint32_t FIBER_POOL_SIZE = 64;

		static const size_t FIBER_STACK_SIZE = 32768; // 0 = binary default
		static const size_t FIBER_STACK_RESERVE_SIZE = 0; // 0 = binary default
		static const size_t MAX_TASKS_PER_QUE = 1024 * 64;
		static const size_t MAX_COUNTERS = 1024;

		static const size_t COUNTER_ALLOCATION_SIZE = sizeof(core::AtomicInt);
		static const size_t FCOUNTER_ALLOCATION_ALIGN = X_ALIGN_OF(core::AtomicInt);

		typedef core::MemoryArena<
			core::PoolAllocator,
			core::MultiThreadPolicy<core::Spinlock>,

#if X_ENABLE_MEMORY_DEBUG_POLICIES
			core::SimpleBoundsChecking,
			core::SimpleMemoryTracking,
			core::SimpleMemoryTagging
#else
			core::NoBoundsChecking,
			core::NoMemoryTracking,
			core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
		> CounterArena;

	public:
		Scheduler();
		~Scheduler();

		bool StartUp(void);
		void ShutDown(void);

		size_t NumThreads(void) const;

		void AddTask(Task task, core::AtomicInt** pCounterOut, JobPriority::Enum priority = JobPriority::NORMAL);
		void AddTasks(Task* pTasks, size_t numTasks, core::AtomicInt** pCounterOut, JobPriority::Enum priority = JobPriority::NORMAL);

		void WaitForCounter(core::AtomicInt* pCounter, int32_t value);
		void WaitForCounterAndFree(core::AtomicInt* pCounter, int32_t value);
		void FreeCounter(core::AtomicInt* pCounter);

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

		TaskQue tasks_[JobPriority::ENUM_COUNT];

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

	private:
		core::HeapArea      counterPoolHeap_;
		core::PoolAllocator counterPoolAllocator_;
		CounterArena		counterPoolArena_;
	};

	X_ENABLE_WARNING(4324)


} // namespace Fiber

X_NAMESPACE_END

#include "FiberScheduler.inl"