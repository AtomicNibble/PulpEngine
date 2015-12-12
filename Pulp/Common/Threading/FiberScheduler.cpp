#include <EngineCommon.h>
#include "FiberScheduler.h"

#include "Util\Cpu.h"

#include "String\StackString.h"


X_NAMESPACE_BEGIN(core)

namespace Fiber
{
	namespace
	{

		ThreadLocalStorage tlsDestFiber;
		ThreadLocalStorage tlsOriginFiber;
		ThreadLocalStorage tlsWaitCounter;
		ThreadLocalStorage tlsWaitValue;



	} // namespace


	Task::Task()
	{
		Function = nullptr;
		pArgData = nullptr;
	}

	Task::Task(TaskFunction::Pointer Function, void* pArgData) :
		Function(Function),
		pArgData(pArgData)
	{

	}

	// =============================

	TaskBundle::TaskBundle()
	{
		pCounter = nullptr;
	}


	TaskBundle::TaskBundle(Task task, core::AtomicInt* counter) :
		task(task), pCounter(counter)
	{

	}

	// =============================

	WaitingTask::WaitingTask()
	{
		fiber = nullptr;
		pCounter = nullptr;
		val = -1;
	}

	// =============================

	Scheduler::Scheduler() : 
		stop_(0),
		activeWorkers_(0),
		numThreads_(0), 
		startUpThreadId_(0),
		waitingTasks_(gEnv->pArena),
		threadToFiberIndex_(gEnv->pArena),
		fiberSwitchingFibers_(gEnv->pArena),
		counterWaitingFibers_(gEnv->pArena),

		tasks_(gEnv->pArena, MAX_TASKS),
		fibers_(gEnv->pArena, FIBER_POOL_SIZE)
	{

	}

	Scheduler::~Scheduler()
	{

	}


	bool Scheduler::StartUp(void)
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);
		X_ASSERT_NOT_NULL(gEnv->pCore);
		X_ASSERT_NOT_NULL(gEnv->pConsole);

		// get the num HW threads
		ICore* pCore = gEnv->pCore;
		CpuInfo* pCpu = pCore->GetCPUInfo();

		uint32_t numCores = pCpu->GetCoreCount();
		numThreads_ = core::Max(core::Min(HW_THREAD_MAX, numCores - HW_THREAD_NUM_DELTA), 1u);

		startUpThreadId_ = core::Thread::GetCurrentID();

		if (!StartThreads()) {
			X_ERROR("Scheduler", "Failed to start worker threads");
			return false;
		}
		if (!CreateFibers()) {
			X_ERROR("Scheduler", "Failed to create fibers");
			return false;
		}

return true;
	}

	void Scheduler::ShutDown(void)
	{
		X_LOG0("Scheduler", "Shutting Down");

		++stop_;

		Fiber::ConvertFiberToThread();

		uint32_t curThreadId = core::Thread::GetCurrentID();

		uint32_t i;
		for (i = 0; i < numThreads_; i++) {
			threads_[i].Stop();
		}

		if (startUpThreadId_ != curThreadId)
		{
			// we ended up on a diffrent thread.
			// this happens when the main thread waited for a counter and then once 
			// counter reached zero got picked up by a diffrent thread than what it was on before.

			// wait for them to exit out.
			while (activeWorkers_ > 0) {
				Thread::Yield();
			}

			// call join on any that are not this one.
			for (i = 0; i < numThreads_; i++) {
				if (threads_[i].GetID() != curThreadId) {
					threads_[i].Join();
				}
			}

			// update thread name
			core::Thread::SetName(curThreadId, "MainThread");

			X_LOG2("Scheduler", "Main thread switched from: ^60x%x^7 to ^60x%x",
				startUpThreadId_, curThreadId);
		}
		else
		{
			for (i = 0; i < numThreads_; i++) {
				threads_[i].Join();
			}
		}

		// fiber clean up.
		{
			FiberHandle fiber;
			while (fibers_.tryPop(fiber)) {
				Fiber::DeleteFiber(fiber);
			}
		}

		for (auto fiber : fiberSwitchingFibers_) {
			Fiber::DeleteFiber(fiber);
		}
		for (auto fiber : counterWaitingFibers_) {
			Fiber::DeleteFiber(fiber);
		}

		threadToFiberIndex_.free();
		fiberSwitchingFibers_.free();
		counterWaitingFibers_.free();
	}


	void Scheduler::AddTask(Task task, core::AtomicInt** pCounterOut)
	{
		if (*pCounterOut == nullptr) {
			*pCounterOut = X_NEW(core::AtomicInt, pAtomicArean_, "Fiber::Counter");
		}

		(**pCounterOut) = 1;


		TaskBundle bundle = { task, *pCounterOut };
		tasks_.Add(bundle);
	}


	void Scheduler::AddTasks(Task* pTasks, size_t numTasks, core::AtomicInt** pCounterOut)
	{
		if (*pCounterOut == nullptr) {
			*pCounterOut = X_NEW(core::AtomicInt, pAtomicArean_, "Fiber::Counter");
		}

		(**pCounterOut) = safe_static_cast<int32_t, size_t>(numTasks);

		if (numTasks < 1) {
			return;
		}

		tasks_.Lock();


		if ((tasks_.numItems() + numTasks) > MAX_TASKS) {
			X_ERROR("Scheduler", "Failed to add %i tasks, not enough space in task que", numTasks);
			return;
		}

		for (size_t i = 0; i < numTasks; i++)
		{
			TaskBundle bundle = { pTasks[i], *pCounterOut };
			tasks_.Add_nolock(bundle);
		}

		tasks_.Unlock();
	}

	void Scheduler::WaitForCounter(core::AtomicInt* pCounter, int32_t value)
	{
		if ((*pCounter) == value) {
			return;
		}

		{
			// switch to a new fiber to do some other work.
			FiberHandle newFiber;
			fibers_.Pop(newFiber);

			FiberHandle curFiber = Fiber::GetCurrentFiber();

			tlsOriginFiber.SetValue(curFiber);
			tlsDestFiber.SetValue(newFiber);
			tlsWaitCounter.SetValue(pCounter);
			tlsWaitValue.SetValue(union_cast<void*, int32_t>(value));
		}

		FiberHandle switchFiber = GetWaitFiberForThread();
		Fiber::SwitchToFiber(switchFiber);
	}

	void Scheduler::WaitForCounterAndFree(core::AtomicInt* pCounter, int32_t value)
	{
		if ((*pCounter) == value) {
			return;
		}

		{
			// switch to a new fiber to do some other work.
			FiberHandle newFiber;
			fibers_.Pop(newFiber);

			FiberHandle curFiber = Fiber::GetCurrentFiber();

			tlsOriginFiber.SetValue(curFiber);
			tlsDestFiber.SetValue(newFiber);
			tlsWaitCounter.SetValue(pCounter);
			tlsWaitValue.SetValue(union_cast<void*, int32_t>(value));
		}

		FiberHandle switchFiber = GetWaitFiberForThread();
		Fiber::SwitchToFiber(switchFiber);

		FreeCounter(pCounter);
	}

	void Scheduler::FreeCounter(core::AtomicInt* pCounter)
	{
		X_DELETE(pCounter, pAtomicArean_);
	}

	bool Scheduler::CreateFibers(void)
	{
		threadToFiberIndex_.reserve(numThreads_);
		fiberSwitchingFibers_.reserve(numThreads_);
		counterWaitingFibers_.reserve(numThreads_);

		uint32_t i;
		for (i = 0; i < FIBER_POOL_SIZE; i++)
		{
			Fiber::FiberHandle newFiber = Fiber::CreateFiber(FIBER_STACK_SIZE, FIBER_STACK_SIZE, FiberStart,
				reinterpret_cast<void*>(this));

			fibers_.Add(newFiber);
		}

		for (i = 0; i < numThreads_; i++)
		{
			uint32_t threadId = threads_[i].GetID();
			CreateFibersForThread(threadId);
		}

		mainThreadFiber_ = Fiber::ConvertThreadToFiber(nullptr);

		// make switch fibers for main thread
		CreateFibersForThread(Thread::GetCurrentID());

		return mainThreadFiber_ != Fiber::InvalidFiberHandle;
	}

	bool Scheduler::CreateFibersForThread(uint32_t threadId)
	{
		threadToFiberIndex_.append(std::make_pair(threadId, fiberSwitchingFibers_.size()));

		fiberSwitchingFibers_.append(
			Fiber::CreateFiber(FIBER_STACK_SIZE, FIBER_STACK_SIZE, FiberSwitchStart, this)
		);
		counterWaitingFibers_.append(
			Fiber::CreateFiber(FIBER_STACK_SIZE, FIBER_STACK_SIZE, CounterWaitStart, this)
		);

		return true;
	}

	bool Scheduler::StartThreads(void)
	{
		X_LOG0("Scheduler", "Creating %i threads", numThreads_);

		uint32_t i;
		for (i = 0; i < numThreads_; i++)
		{
			core::StackString<64> name;
			name.appendFmt("Fiber::Scheduler::Worker_%i", i);
			threads_[i].setData(this);
			threads_[i].Create(name.c_str()); // default stack size.
			threads_[i].Start(ThreadRun);
		}
		
		activeWorkers_ = numThreads_;
		return true;
	}

	bool Scheduler::GetTask(TaskBundle& task)
	{
		return tasks_.tryPop(task);
	}

	void Scheduler::SwitchFibers(FiberHandle fiberToSwitchTo)
	{
		{
			FiberHandle curFiber = Fiber::GetCurrentFiber();

			tlsOriginFiber.SetValue(curFiber);
			tlsDestFiber.SetValue(fiberToSwitchTo);
		}

		FiberHandle switchFiber = GetSwitchFiberForThread();
		Fiber::SwitchToFiber(switchFiber);
	}

	size_t Scheduler::GetCurrentThreadIndex(void) const
	{
		uint32_t threadId = Thread::GetCurrentID();

		for (size_t i = 0; i < threadToFiberIndex_.size(); i++)
		{
			if (threadToFiberIndex_[i].first == threadId) {
				return threadToFiberIndex_[i].second; // we could just return i.
			}
		}

		X_ASSERT_UNREACHABLE();
		return 0;
	}

	FiberHandle Scheduler::GetSwitchFiberForThread(void) const
	{
		size_t idx = GetCurrentThreadIndex();

		return fiberSwitchingFibers_[idx];
	}

	FiberHandle Scheduler::GetWaitFiberForThread(void) const
	{
		size_t idx = GetCurrentThreadIndex();

		return counterWaitingFibers_[idx];
	}

	Thread::ReturnValue Scheduler::ThreadRun(const Thread& thread)
	{
		Thread::ReturnValue retVal = Thread::ReturnValue(0);

		// convert to fiber
		Fiber::ConvertThreadToFiber(nullptr);

		FiberStart(thread.getData());

		return retVal;
	}

	void Scheduler::FiberStart(void* pArg)
	{
		Scheduler* pScheduler = static_cast<Scheduler*>(pArg);

		core::AtomicInt& stop = pScheduler->stop_;

		while (stop == 0)
		{

			// check for any waiting tasks that are ready
			WaitingTaskArr& waitingTasks = pScheduler->waitingTasks_;
			if (waitingTasks.isNotEmpty())
			{
				WaitingTask waitingTask;
				bool waitingTaskReady = false;

				{
					Spinlock::ScopedLock lock(pScheduler->waitingTaskLock_);

					WaitingTaskArr::Iterator it = waitingTasks.begin();
					for (; it != waitingTasks.end(); ++it)
					{
						core::AtomicInt& counter = *it->pCounter;

						if (counter == it->val)
						{
							waitingTaskReady = true;
							break;
						}
					}

					// remove it from list.
					if (waitingTaskReady)
					{
						waitingTask = *it;

						WaitingTaskArr::Iterator endIt = waitingTasks.end();
						if (it != (--endIt))
						{
							*it = std::move(waitingTasks.back());
						}

						waitingTasks.pop_back();
					}
				}

				// switch to the waitings tasks fiber.
				if (waitingTaskReady)
				{
					pScheduler->SwitchFibers(waitingTask.fiber);
				}
			}


			// get a task.
			TaskBundle curTask;
			if (pScheduler->GetTask(curTask))
			{
				curTask.task.Function(pScheduler, curTask.task.pArgData);
				// dec counter.
				core::AtomicInt& counter = *curTask.pCounter;

				--counter;
			}
			else
			{
				Thread::Yield();
			}
		}
			
		// We haver to do this here and not after returning back to ThreadRun.
		// Since if we switch thread, one thread won't go back into that fucntion after compelting.

		// convert back to thread.
		Fiber::ConvertFiberToThread();
		// dec active workers.
		--pScheduler->activeWorkers_;
	}

	void Scheduler::FiberSwitchStart(void* pArg)
	{
		Scheduler* pScheduler = static_cast<Scheduler*>(pArg);

		// this is entered when we want swtich fiber.
		// we place the fiber we just left back into the pool.
		// and enter then target fiber.

		// we only enter this function once, but just context switch in/out of the loop
		X_DISABLE_WARNING(4127)
		while (true)
		X_ENABLE_WARNING(4127)
		{
			FiberHandle originFiber = tlsOriginFiber.GetValue<FiberHandle>();
			FiberHandle destFiber = tlsDestFiber.GetValue<FiberHandle>();

			// it's not safe to place the fiber we just left into the pool of useable fibers.
			pScheduler->fibers_.Add(originFiber);

			Fiber::SwitchToFiber(destFiber);
		}
	}

	void Scheduler::CounterWaitStart(void* pArg)
	{
		Scheduler* pScheduler = static_cast<Scheduler*>(pArg);

		// we switch to this when we are waiting for a counter.
		// so we can do some work while waiting.

		X_DISABLE_WARNING(4127)
		while (true)
		X_ENABLE_WARNING(4127)
		{
			{
				Spinlock::ScopedLock lock(pScheduler->waitingTaskLock_);

				WaitingTask waitingTask;
				waitingTask.fiber = tlsOriginFiber.GetValue<FiberHandle>();
				waitingTask.pCounter = tlsWaitCounter.GetValue<core::AtomicInt>();
				waitingTask.val = union_cast<int32_t,int32_t*>(tlsWaitValue.GetValue<int32_t>());

				pScheduler->waitingTasks_.append(waitingTask);
			}

			FiberHandle destFiber = tlsDestFiber.GetValue<FiberHandle>();

			Fiber::SwitchToFiber(destFiber);
		}
	}


} // namespadce Fiber


X_NAMESPACE_END