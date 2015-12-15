#pragma once

#include <Memory\ThreadPolicies\MultiThreadPolicy.h>
#include <Memory\AllocationPolicies\PoolAllocator.h>
#include <Memory\BoundsCheckingPolicies\NoBoundsChecking.h>
#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>
#include <Memory\MemoryTaggingPolicies\NoMemoryTagging.h>
#include <Memory\HeapArea.h>

#include <Traits\FunctionTraits.h>

#include <Threading\ThreadLocalStorage.h>

#include <Containers\FixedArray.h>
#include <Containers\Freelist.h>

X_NAMESPACE_BEGIN(core)

namespace V2
{ 

/*

For this one i'm going to have ques for each thread.
And job stealing from other threads ques.

Jobs are added to the que of the thread calling Add.

I think i'll have a fixed amount of jobs per a frame
THen between each frame call reset?

But that won't allow for multiple frames to be worked on at the same time.

THis would not be a issue if the ques stored the job not just a job pointer.
But that would mean once a job is submitted can't get it back.

Lets work out how to delete the jobs later.


*/

class JobSystem;
struct Job;


typedef core::traits::Function<void(JobSystem*, size_t, Job*, void*)> JobFunction;

struct Job
{
	JobFunction::Pointer pFunction;
	Job* pParent;
	void* pArgData;
	core::AtomicInt unfinishedJobs;

#if X_64
	char pad[(50 - 4) - 12];
#else
	char pad[50 - 4];
#endif
};

X_ENSURE_SIZE(Job, 64);

X_DISABLE_WARNING(4324)
X_ALIGNED_SYMBOL(class ThreadQue, 64)
{
	static const unsigned int MAX_NUMBER_OF_JOBS = 1 << 16;
	static const unsigned int MASK = MAX_NUMBER_OF_JOBS - 1u;

public:
	ThreadQue();

	X_INLINE void Push(Job* job);
	X_INLINE Job* Pop(void);
	X_INLINE Job* Steal(void);

private:
	long bottom_;
	long top_;

	Job* jobs_[MAX_NUMBER_OF_JOBS];
};


struct JobID
{
	uint32_t jobOffset;
};

class JobSystem
{
	struct ThreadJobAllocator;

public:
	static const uint32_t HW_THREAD_MAX = 16; // max even if hardware supports more.
	static const uint32_t HW_THREAD_NUM_DELTA = 1; // num = Min(max,hw_num-delta);
	static const size_t MAX_JOBS = 1 << 16;

public:
	JobSystem();
	~JobSystem();

	bool Start(void);
	void ShutDown(void);

private:
	bool StartThreads(void);
	void CreateThreadObjects(uint32_t threadId);

public:
	Job* CreateJob(JobFunction::Pointer function);
	Job* CreateJob(JobFunction::Pointer function, void* pData);
	Job* CreateJobAsChild(Job* pParent, JobFunction::Pointer function);
	Job* CreateJobAsChild(Job* pParent, JobFunction::Pointer function, void* pData);

	void Run(Job* pJob);
	void Wait(Job* pJob);
	void WaitWithoutHelp(Job* pJob);

private:
	Job* AllocateJob(void);
	Job* AllocateJob(size_t threadIdx);
	void FreeJob(Job* pJob);

	ThreadQue* GetWorkerThreadQueue(void) const;
	ThreadQue* GetWorkerThreadQueue(size_t threadIdx) const;
	ThreadJobAllocator* GetWorkerThreadAllocator(void) const;
	ThreadJobAllocator* GetWorkerThreadAllocator(size_t threadIdx) const;

	X_INLINE bool IsEmptyJob(Job* pJob) const;
	X_INLINE bool HasJobCompleted(Job* pJob) const;

	Job* GetJob(void);
	Job* GetJob(ThreadQue& queue);
	void Execute(Job* pJob, size_t theadIdx);
	void Finish(Job* pJob) const;

	size_t GetThreadIndex(void) const;

	static void ThreadBackOff(int32_t backoff);
	static Thread::ReturnValue ThreadRun_s(const Thread& thread);
	Thread::ReturnValue ThreadRun(const Thread& thread);

private:
	typedef core::FixedArray<std::pair<uint32_t, size_t>, HW_THREAD_MAX > ThreadIdToIndex;
	typedef core::MemoryArena<
		core::PoolAllocator,
		core::SingleThreadPolicy,
		core::NoBoundsChecking,
		core::NoMemoryTracking,
		core::NoMemoryTagging> JobArena;

	Thread threads_[HW_THREAD_MAX];
	ThreadIdToIndex threadIdToIndex_;
	uint32_t numThreads_;

	ThreadQue* pThreadQues_[HW_THREAD_MAX];

private:
	X_ALIGNED_SYMBOL(struct ThreadJobAllocator, 64)
	{
		ThreadJobAllocator();

#if 0
		core::HeapArea      jobPoolHeap;
		core::PoolAllocator jobPoolAllocator;
		JobArena			jobPoolArena;
#endif
		uint32_t allocated;

		Job jobs[JobSystem::MAX_JOBS];
	};


	ThreadJobAllocator* pJobAllocators_[HW_THREAD_MAX];
};

X_ENABLE_WARNING(4324)

} // namespace V2

X_NAMESPACE_END

#include "JobSystem2.inl"
