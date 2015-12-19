#include "EngineCommon.h"
#include "JobSystem2.h"

#include "Random\MultiplyWithCarry.h"
#include "Random\XorShift.h"

#include "Util\Cpu.h"
#include <Memory\VirtualMem.h>

X_NAMESPACE_BEGIN(core)


namespace V2
{
	namespace
	{
		ThreadLocalStorage gThreadQue;
		ThreadLocalStorage gThreadAllocator;

		#define COMPILER_BARRIER_R _ReadBarrier();
		#define COMPILER_BARRIER_W _WriteBarrier();
		#define COMPILER_BARRIER_RW _ReadWriteBarrier();

	}


	// ===================================

	ThreadQue::ThreadQue()
	{
		bottom_ = 0;
		top_ = 0;
	}

	void ThreadQue::Push(Job* job)
	{
		long b = bottom_;
		jobs_[b & MASK] = job;

		// ensure the job is written before b+1 is published to other threads.
		COMPILER_BARRIER_W;

		bottom_ = b + 1;
	}

	Job* ThreadQue::Pop(void)
	{
		long b = bottom_ - 1;

		atomic::Exchange(&bottom_, b);

		long t = top_;
		if (t <= b)
		{
			// non-empty queue
			Job* job = jobs_[b & MASK];
			if (t != b)
			{
				// there's still more than one item left in the queue
				return job;
			}

			// this is the last item in the queue
			if (atomic::CompareExchange(&top_, t + 1, t) != t)
			{
				// failed race against steal operation
				job = nullptr;
			}

			bottom_ = t + 1;
			return job;
		}
		else
		{
			// deque was already empty
			bottom_ = t;
			return nullptr;
		}
	}

	Job* ThreadQue::Steal(void)
	{
		long t = top_;

		// ensure that top is always read before bottom.
		// loads will not be reordered with other loads on x86, so a compiler barrier is enough.
		COMPILER_BARRIER_R;

		long b = bottom_;
		if (t < b)
		{
			// non-empty queue
			Job* job = jobs_[t & MASK];

			// the interlocked function serves as a compiler barrier, and guarantees that the read happens before the CAS.
			if (atomic::CompareExchange(&top_, t + 1, t) != t)
			{
				// a concurrent steal or pop operation removed an element from the deque in the meantime.
				return nullptr;
			}
			return job;
		}
		else
		{
			// empty queue
			return nullptr;
		}
	}


	// ===================================

	JobSystem::ThreadJobAllocator::ThreadJobAllocator() 
#if 0
		: jobPoolHeap(
			bitUtil::RoundUpToMultiple<size_t>(
				JobArena::getMemoryRequirement(sizeof(Job)) * MAX_JOBS,
				VirtualMem::GetPageSize()
				)
			),
		jobPoolAllocator(jobPoolHeap.start(), jobPoolHeap.end(),
			JobArena::getMemoryRequirement(sizeof(Job)),
			JobArena::getMemoryAlignmentRequirement(X_ALIGN_OF(Job)),
			JobArena::getMemoryOffsetRequirement()
			),
		jobPoolArena(&jobPoolAllocator, "JobPool")
#endif
	{
		allocated = 0;
	}

	JobSystem::JobSystem() 
	{
		numThreads_ = 0;
		core::zero_object(pThreadQues_);
		core::zero_object(pJobAllocators_);
	}

	JobSystem::~JobSystem()
	{

	}



	bool JobSystem::StartUp(void)
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);
		X_ASSERT_NOT_NULL(gEnv->pCore);
		X_ASSERT_NOT_NULL(gEnv->pConsole);

		ICore* pCore = gEnv->pCore;
		CpuInfo* pCpu = pCore->GetCPUInfo();

		uint32_t numCores = pCpu->GetCoreCount();
		numThreads_ = core::Max(core::Min(HW_THREAD_MAX, numCores - HW_THREAD_NUM_DELTA), 1u);


		if (!StartThreads()) {
			X_ERROR("Scheduler", "Failed to start worker threads");
			return false;
		}

		return true;
	}

	void JobSystem::ShutDown(void)
	{
		size_t i;
		for (i = 0; i < numThreads_; i++)
		{
			threads_[i].Stop();
		}
		for (i = 0; i < numThreads_; i++)
		{
			threads_[i].Join();
		}

		// clean up allocators and ques.
		for (i = 0; i < HW_THREAD_MAX; i++)
		{
			if (pThreadQues_[i]) {
				X_DELETE(pThreadQues_[i], gEnv->pArena);
			}
			if (pJobAllocators_[i]) {
				X_DELETE(pJobAllocators_[i], gEnv->pArena);
			}
		}

		threadIdToIndex_.clear();

		core::zero_object(pThreadQues_);
		core::zero_object(pJobAllocators_);

		// clear main thread tls values also.
		gThreadQue.SetValue(nullptr);
		gThreadAllocator.SetValue(nullptr);
	}

	/// ===============================================

	void JobSystem::CreateQueForCurrentThread(void)
	{
		uint32_t threadId = Thread::GetCurrentID();

		CreateThreadObjects(threadId);
		{
			size_t threadIdx = GetThreadIndex();
			gThreadQue.SetValue(pThreadQues_[threadIdx]);
			gThreadAllocator.SetValue(pJobAllocators_[threadIdx]);
		}

		numThreads_++;
	}

	/// ===============================================


	bool JobSystem::StartThreads(void)
	{
		X_LOG0("JobSystemV2", "Creating %i threads", numThreads_);

		// que for main thread also.
		uint32_t threadId = Thread::GetCurrentID();

		CreateThreadObjects(threadId);
		{
			size_t threadIdx = GetThreadIndex();
			gThreadQue.SetValue(pThreadQues_[threadIdx]);
			gThreadAllocator.SetValue(pJobAllocators_[threadIdx]);
		}

		uint32_t i;
		for (i = 0; i < numThreads_; i++)
		{
			core::StackString<64> name;
			name.appendFmt("JobSystemV2::Worker_%i", i);
			threads_[i].setData(this);
			threads_[i].Create(name.c_str()); // default stack size.

			threadId = threads_[i].GetID();
			CreateThreadObjects(threadId);
		}
		for (i = 0; i < numThreads_; i++)
		{
			threads_[i].Start(ThreadRun_s);
		}

		return true;
	}

	void JobSystem::CreateThreadObjects(uint32_t threadId)
	{
		uint32_t idx = safe_static_cast<uint32_t, size_t>(threadIdToIndex_.size());
		pThreadQues_[idx] = X_NEW(ThreadQue, gEnv->pArena, "JobThreadQue");
		pJobAllocators_[idx] = X_NEW(ThreadJobAllocator, gEnv->pArena, "JobThreadAllocator");
		threadIdToIndex_.push_back(std::make_pair(threadId, idx));
	}


	/// ===============================================


	Job* JobSystem::CreateJob(JobFunction::Pointer function)
	{
		Job* job = AllocateJob();
		job->pFunction = function;
		job->pParent = nullptr;
		job->unfinishedJobs = 1;
		return job;
	}

	Job* JobSystem::CreateJobAsChild(Job* pParent, JobFunction::Pointer function)
	{
		++pParent->unfinishedJobs;

		Job* job = AllocateJob();
		job->pFunction = function;
		job->pParent = pParent;
		job->unfinishedJobs = 1;
		return job;
	}

	/// ===============================================

	void JobSystem::Run(Job* pJob)
	{
		ThreadQue* queue = GetWorkerThreadQueue();
		queue->Push(pJob);
	}

	void JobSystem::Wait(Job* pJob)
	{
		size_t threadIdx = GetThreadIndex();
		ThreadQue& threadQue = *GetWorkerThreadQueue(threadIdx);

		while (!HasJobCompleted(pJob))
		{
			Job* nextJob = GetJob(threadQue);
			if (nextJob)
			{
				Execute(nextJob, threadIdx);
			}
		}
	}

	void JobSystem::WaitWithoutHelp(Job* pJob) const 
	{
		while (!HasJobCompleted(pJob))
		{
			Thread::Yield();
		}
	}

	/// ===============================================

	Job* JobSystem::AllocateJob(void)
	{
		ThreadJobAllocator* pAllocator = GetWorkerThreadAllocator();

		const uint32_t index = pAllocator->allocated++;
		return &pAllocator->jobs[index & (JobSystem::MAX_JOBS - 1u)];
	}

	Job* JobSystem::AllocateJob(size_t threadIdx)
	{
		ThreadJobAllocator* pAllocator = GetWorkerThreadAllocator(threadIdx);

		const uint32_t index = pAllocator->allocated++;
		return &pAllocator->jobs[index & (JobSystem::MAX_JOBS - 1u)];
	}

	void JobSystem::FreeJob(Job* pJob)
	{
		X_UNUSED(pJob);
		X_ASSERT_NOT_IMPLEMENTED();
	}

	/// ===============================================

	ThreadQue* JobSystem::GetWorkerThreadQueue(void) const
	{
		return gThreadQue.GetValue<ThreadQue>();
	}

	ThreadQue* JobSystem::GetWorkerThreadQueue(size_t threadIdx) const
	{
		return pThreadQues_[threadIdx];
	}

	JobSystem::ThreadJobAllocator* JobSystem::GetWorkerThreadAllocator(void) const
	{
		return gThreadAllocator.GetValue<ThreadJobAllocator>();
	}

	JobSystem::ThreadJobAllocator* JobSystem::GetWorkerThreadAllocator(size_t threadIdx) const
	{
		return pJobAllocators_[threadIdx];
	}

	/// ===============================================

	Job* JobSystem::GetJob(void)
	{
		ThreadQue* queue = GetWorkerThreadQueue();
		return GetJob(*queue);
	}

	Job* JobSystem::GetJob(ThreadQue& queue)
	{
		Job* job = queue.Pop();
		if (IsEmptyJob(job))
		{
			// this is not a valid job because our own queue is empty, so try stealing from some other queue
			uint32_t randomIndex = random::MultiplyWithCarry(0u, numThreads_ + 1);

			ThreadQue* stealQueue = pThreadQues_[randomIndex];
			if (stealQueue == &queue)
			{
				// don't try to steal from ourselves
				// but if we not main que try steal from that
				return nullptr;
			}

			Job* stolenJob = stealQueue->Steal();
			if (IsEmptyJob(stolenJob))
			{
				// we couldn't steal a job from the other queue either, so we just yield our time slice for now
				return nullptr;
			}

			return stolenJob;
		}

		return job;
	}


	void JobSystem::Execute(Job* pJob, size_t threadIdx) 
	{
		(pJob->pFunction)(this, threadIdx, pJob, pJob->pArgData);
		Finish(pJob);
	}

	void JobSystem::Finish(Job* pJob) const
	{
		const int32_t unfinishedJobs = --pJob->unfinishedJobs;
		if (unfinishedJobs == 0 && pJob->pParent)
		{
			Finish(pJob->pParent);
		}
	}


	size_t JobSystem::GetThreadIndex(void) const
	{
		uint32_t threadId = Thread::GetCurrentID();

		for (auto id : threadIdToIndex_) {
			if (id.first == threadId) {
				return id.second;
			}
		}

		X_ASSERT_UNREACHABLE();
		return 0;
	}

	Thread::ReturnValue JobSystem::ThreadRun_s(const Thread& thread)
	{
		JobSystem* pJobSystem = static_cast<JobSystem*>(thread.getData());
		return pJobSystem->ThreadRun(thread);
	}


	void JobSystem::ThreadBackOff(int32_t backoff)
	{
		if (backoff < 10 && backoff > 0) {
			Thread::YieldProcessor();
		}
		else if (backoff < 20) {
			for (size_t i = 0; i != 50; i += 1) {
				Thread::YieldProcessor();
			}
		}
		else if (backoff < 28) {
			Thread::Yield();
		}
		else if (backoff < 64) {
			Thread::Sleep(0);
		}
		else if (backoff < 80) {
			Thread::Sleep(1);
		}
		else {
			Thread::Sleep(5);
		}
	}

	Thread::ReturnValue JobSystem::ThreadRun(const Thread& thread)
	{
		size_t threadIdx = GetThreadIndex();
		ThreadQue& threadQue = *GetWorkerThreadQueue(threadIdx);
		ThreadJobAllocator* pThreadAlloc = GetWorkerThreadAllocator(threadIdx);

		gThreadQue.SetValue(&threadQue);
		gThreadAllocator.SetValue(pThreadAlloc);

		int32_t backoff = 0;

		while (thread.ShouldRun())
		{
			Job* job = GetJob(threadQue);
			if (job)
			{
				backoff = 0;
				Execute(job, threadIdx);
			}
			else
			{
				ThreadBackOff(backoff);
				backoff++;
			}
		}

		// null the tls (not needed)
		gThreadQue.SetValue(nullptr);
		gThreadAllocator.SetValue(nullptr);

		return Thread::ReturnValue(0);
	}



} // namespace V2


X_NAMESPACE_END
