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
	static const size_t PAD_SIZE = (64 - ((sizeof(void*) * 3) + (sizeof(core::AtomicInt) + sizeof(int32_t))));
public:
	core::AtomicInt unfinishedJobs;
	int32_t pad_;
	JobFunction::Pointer pFunction;
	Job* pParent;
	void* pArgData;

	union
	{
		char pad[PAD_SIZE];
	};
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


class CountSplitter
{
public:
	explicit CountSplitter(size_t count);

	template <typename T>
	X_INLINE bool Split(size_t count) const;

private:
	size_t count_;
};

class DataSizeSplitter
{
public:
	explicit DataSizeSplitter(size_t size);

	template <typename T>
	X_INLINE bool Split(size_t count) const;

private:
	size_t size_;
};


template <typename JobData>
static inline void parallel_for_job(JobSystem* pJobSys, size_t threadIdx, Job* job, void* jobData)
{
	const JobData* data = static_cast<const JobData*>(jobData);
	const JobData::SplitterType& splitter = data->splitter;

	if (splitter.Split<JobData::DataType>(data->count))
	{
		// split in two
		const size_t leftCount = data->count / 2u;
		const JobData leftData(data->data, leftCount, data->pFunction, splitter);
		Job* left = pJobSys->CreateJobAsChild(job, &parallel_for_job<JobData>, leftData);
		pJobSys->Run(left);

		const size_t rightCount = data->count - leftCount;
		const JobData rightData(data->data + leftCount, rightCount, data->pFunction, splitter);
		Job* right = pJobSys->CreateJobAsChild(job, &parallel_for_job<JobData>, rightData);
		pJobSys->Run(right);
	}
	else
	{
		(data->pFunction)(data->data, data->count);
	}
}


template <typename T, typename S>
struct parallel_for_job_data
{
	typedef T DataType;
	typedef S SplitterType;
	typedef traits::Function<void(DataType*, size_t)> DataJobFunction;
	typedef typename DataJobFunction::Pointer DataJobFunctionPtr;

	parallel_for_job_data(DataType* data, size_t count, DataJobFunctionPtr function, const SplitterType& splitter): 
		data(data)
		, count(count)
		, pFunction(function)
		, splitter(splitter)
	{
	}

	DataType* data;
	size_t count;
	DataJobFunctionPtr pFunction;
	SplitterType splitter;
};

template <typename JobData>
static inline void member_function_job(JobSystem* pJobSys, size_t threadIdx, Job* job, void* jobData)
{
	JobData* data = static_cast<JobData*>(jobData);
	(*data->pInst.*data->pFunction)(pJobSys, threadIdx, job, data->pJobData);
}


template<typename C>
struct member_function_job_data
{
	typedef C ClassType;
	typedef traits::MemberFunction<C, void(JobSystem*, size_t, Job*, void*)> MemberFunction;
	typedef typename MemberFunction::Pointer MemberFunctionPtr;

	member_function_job_data(ClassType* pInst, MemberFunctionPtr function, void* jobData) :
		pInst(pInst),
		pFunction(function),
		pJobData(jobData)
	{

	}

	ClassType* pInst;
	MemberFunctionPtr pFunction;
	void* pJobData;
};

class JobSystem
{
	struct ThreadJobAllocator;

public:
	static const uint32_t HW_THREAD_MAX = 16; // max even if hardware supports more.
	static const uint32_t HW_THREAD_NUM_DELTA = 1; // num = Min(max,hw_num-delta);
	static const size_t MAX_JOBS = 1 << 12;
	// 68kb(72kb x64) per 1024 jobs per thread
	// so for 4096 jobs with 6 threads it's: 1728kb ~1.7mb
	static const size_t MEMORY_PER_THREAD = (MAX_JOBS * sizeof(Job)) + (MAX_JOBS * sizeof(Job*));
public:
	JobSystem();
	~JobSystem();

	bool StartUp(void);
	void ShutDown(void);

	void CreateQueForCurrentThread(void);

private:
	bool StartThreads(void);
	void CreateThreadObjects(uint32_t threadId);

public:
	Job* CreateJob(JobFunction::Pointer function);
	Job* CreateJobAsChild(Job* pParent, JobFunction::Pointer function);

	X_INLINE Job* CreateJob(JobFunction::Pointer function, void* pData);
	X_INLINE Job* CreateJobAsChild(Job* pParent, JobFunction::Pointer function, void* pData);

	template<typename T>
	X_INLINE Job* CreateJob(JobFunction::Pointer function, const T& data);
	template<typename T>
	X_INLINE Job* CreateJobAsChild(Job* pParent, JobFunction::Pointer function, const T& data);

	template <typename T, typename SplitterT>
	X_INLINE Job* parallel_for(T* data, size_t count, 
		typename parallel_for_job_data<T,SplitterT>::DataJobFunctionPtr function, const SplitterT& splitter);

	template<typename ClassType>
	X_INLINE Job* CreateJobMemberFunc(ClassType* pInst, typename member_function_job_data<ClassType>::MemberFunctionPtr pFunction, 
		void* pJobData);


	void Run(Job* pJob);
	void Wait(Job* pJob);
	void WaitWithoutHelp(Job* pJob) const;

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
