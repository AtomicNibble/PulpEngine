#pragma once

#include <Memory\ThreadPolicies\MultiThreadPolicy.h>
#include <Memory\AllocationPolicies\PoolAllocator.h>
#include <Memory\BoundsCheckingPolicies\NoBoundsChecking.h>
#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>
#include <Memory\MemoryTaggingPolicies\NoMemoryTagging.h>
#include <Memory\HeapArea.h>

#include <Traits\FunctionTraits.h>

#include <Threading\ThreadLocalStorage.h>
#include <Threading\ConditionVariable.h>

#include <Containers\FixedArray.h>
#include <Containers\Freelist.h>

#include <Util\Delegate.h>


#if X_ENABLE_JOBSYS_PROFILER
#include <Time\TimeVal.h>

#include <Containers\Array.h>
#endif // !X_ENABLE_JOBSYS_PROFILER


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


typedef core::traits::Function<void(JobSystem&, size_t, Job*, void*)> JobFunction;

X_ALIGNED_SYMBOL(struct Job, 64)
{
	static const size_t THREAD_IDX_BITS = 4; // 12 threads
	static const size_t JOB_IDX_BITS = 12;  // 4096
	static const uint32_t MAX_NUMBER_OF_JOBS = 1 << JOB_IDX_BITS;

	X_PACK_PUSH(2)
	struct JobId
	{
		uint16_t threadIdx : THREAD_IDX_BITS;
		uint16_t jobIdx : JOB_IDX_BITS;
	};
	X_PACK_POP
	X_ENSURE_SIZE(JobId, 2);

	static const size_t MAX_CONTINUATIONS = 8;
	static const size_t PAD_SIZE = (128 - 
		(sizeof(JobFunction::Pointer) 
		+ sizeof(Job*)
		+ sizeof(void*)
		+ (sizeof(core::AtomicInt) * 2)
		+ (sizeof(JobId) * MAX_CONTINUATIONS)
		+ sizeof(profiler::SubSys::Enum)
		+ sizeof(uint8_t)
		+ sizeof(uint8_t)));

public:
	int32_t unfinishedJobs; // not using AtomicInt so Job is pod bitches.
	int32_t continuationCount;
	JobFunction::Pointer pFunction;
	Job* pParent;
	void* pArgData;

	union
	{
		char pad[PAD_SIZE];
	};

#if X_ENABLE_JOBSYS_RECORD_SUBSYSTEM
	profiler::SubSys::Enum subSystem;
#else 
	uint8_t _subSysPad;
#endif // !X_ENABLE_JOBSYS_RECORD_SUBSYSTEM

	uint8_t origThreadIdx;
	uint8_t runFlags;
	JobId continuations[MAX_CONTINUATIONS];
};

static_assert(core::compileTime::IsPOD<Job>::Value, "Job must POD");

X_ENSURE_SIZE(Job, 128);



#if X_ENABLE_JOBSYS_RECORD_SUBSYSTEM

#define JOB_SYS_SUB_ARG_SINGLE(sub) sub
#define JOB_SYS_SUB_ARG(sub) , sub

#define JOB_SYS_SUB_PARAM , profiler::SubSys::Enum subSystem
#define JOB_SYS_SUB_PARAM_SINGLE profiler::SubSys::Enum subSystem
#define JOB_SYS_SUB_PASS(sub) , sub

#else 

#define JOB_SYS_SUB_ARG_SINGLE(sub) 
#define JOB_SYS_SUB_ARG(sub)

#define JOB_SYS_SUB_PARAM 
#define JOB_SYS_SUB_PARAM_SINGLE 
#define JOB_SYS_SUB_PASS(sub) 

#endif // !X_ENABLE_JOBSYS_RECORD_SUBSYSTEM

#if X_ENABLE_JOBSYS_PROFILER

// static const uint32_t MASK = 16;
static const uint32_t JOBSYS_HISTORY_COUNT = 16;
static const uint32_t JOBSYS_HISTORY_MASK = JOBSYS_HISTORY_COUNT - 1;

struct JobSystemStats
{
	void clear(void);

	core::AtomicInt jobsStolen;			// total jobs stolen
	core::AtomicInt jobsRun;			// total jobs run
	core::AtomicInt jobsAssited;		// total jobs run via WaitWithHelp
	core::AtomicInt workerUsedMask;		// mask of which workers ran jobs
	core::AtomicInt workerAwokenMask;	// mask of which works where awoken
};


X_DISABLE_WARNING(4324)
class JobQueueHistory
{
public:
	static const uint32_t MAX_NUMBER_OF_JOBS = Job::MAX_NUMBER_OF_JOBS;
	static const uint32_t MASK = MAX_NUMBER_OF_JOBS - 1u;

public:
	struct Entry
	{
		// 16
		core::TimeVal start;
		core::TimeVal end;

		Job::JobId id; // 2 
		profiler::SubSys::Enum subsystem; // 1
		uint8_t _pad[5];
	};

	X_ENSURE_SIZE(Entry, 24);

	//  24 * 4096 = 98304
	// 98304 / 1024 = 96kb
	// 96kb * 12 = 1.1MB
	// so about 1MB per frame if data for 12 threads.
	X_ALIGNED_SYMBOL(struct FrameHistory, 64)
	{
		FrameHistory();
		X_INLINE const int32_t getMaxreadIdx(void) const;

		long bottom_;
		long top_;

		core::TimeVal start;
		Entry entryes_[MAX_NUMBER_OF_JOBS];
	};

	typedef std::array<FrameHistory, JOBSYS_HISTORY_COUNT> FrameHistoryArr;

public:
	JobQueueHistory();
	~JobQueueHistory();

	void sethistoryIndex(int32_t historyIdx);
	X_INLINE const FrameHistoryArr& getHistory(void) const;

	// called from one thread.
	FrameHistory& getCurFrameHistory(void);

private:
	long currentIdx_;

	FrameHistoryArr frameHistory_;
};
X_ENABLE_WARNING(4324)

#endif // !X_ENABLE_JOBSYS_PROFILER


X_DISABLE_WARNING(4324)

X_ALIGNED_SYMBOL(class ThreadQue, 64)
{
public:
	static const uint32_t MAX_NUMBER_OF_JOBS = Job::MAX_NUMBER_OF_JOBS;
private:
	static const uint32_t MASK = MAX_NUMBER_OF_JOBS - 1u;

public:
	ThreadQue();

	X_INLINE void Push(Job* job);
	X_INLINE Job* Pop(void);
	X_INLINE Job* Steal(void);

	X_INLINE size_t rand(void);

private:
	long bottom_;
	long top_;

	// each queue has it's own rand gen, to prevent thread sharing
	// plus they are not thread safe, so won't behave normally when called with multiple threads.
	// i've tried padding around this and it makes it worse, so leave it in same cache lane as bottom & top.
#if X_64
	core::random::XorShift128 rand_;
#else
	core::random::XorShift rand_;
#endif // !X_64

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

class CountSplitter32
{
public:
	explicit CountSplitter32(uint32_t count);

	template <typename T>
	X_INLINE bool Split(uint32_t count) const;

private:
	uint32_t count_;
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


template <typename T, typename S>
struct parallel_for_job_data
{
	typedef T DataType;
	typedef S SplitterType;
	typedef traits::Function<void(DataType*, size_t)> DataJobFunction;
	typedef typename DataJobFunction::Pointer DataJobFunctionPtr;

	parallel_for_job_data(DataType* data, size_t count, DataJobFunctionPtr function, const SplitterType& splitter): 
		data_(data),
		count_(count),
		pFunction_(function),
		splitter_(splitter)
	{
	}

	DataType* data_;
	size_t count_;
	DataJobFunctionPtr pFunction_;
	SplitterType splitter_;
};


template <typename C, typename T, typename S>
struct parallel_for_member_job_data
{
	typedef C ClassType;
	typedef T DataType;
	typedef S SplitterType;
	typedef traits::MemberFunction<C, void(DataType*, uint32_t)> DataJobMemberFunction;
	typedef typename DataJobMemberFunction::Pointer DataJobMemberFunctionPtr;
	typedef core::Delegate<void(DataType*, uint32_t)> FunctionDelagte;

	parallel_for_member_job_data(FunctionDelagte delagte, DataType* data, uint32_t count,
		 const SplitterType& splitter) :
		data_(data),
		delagte_(delagte),
		count_(count),
		splitter_(splitter)
	{

	}

	DataType* data_; 
	FunctionDelagte delagte_;
	uint32_t count_; 
	SplitterType splitter_; 
};

template <typename JobData>
static inline void member_function_job(JobSystem& jobSys, size_t threadIdx, Job* job, void* jobData)
{
	JobData* data = static_cast<JobData*>(jobData);
	(*data->pInst_.*data->pFunction_)(jobSys, threadIdx, job, data->pJobData_);
}


template<typename C>
struct member_function_job_data
{
	typedef C ClassType;
	typedef traits::MemberFunction<C, void(JobSystem&, size_t, Job*, void*)> MemberFunction;
	typedef typename MemberFunction::Pointer MemberFunctionPtr;

	member_function_job_data(ClassType* pInst, MemberFunctionPtr function, void* jobData) :
		pInst_(pInst),
		pFunction_(function),
		pJobData_(jobData)
	{

	}

	ClassType* pInst_;
	MemberFunctionPtr pFunction_;
	void* pJobData_;
};

template <typename JobData>
static inline void member_function_job_copy(JobSystem& jobSys, size_t threadIdx, Job* job, void* jobData)
{
	JobData* pData = static_cast<JobData*>(jobData);
	(*pData->pInst_.*pData->pFunction_)(jobSys, threadIdx, job, pData->data_);
}


template<typename C, typename DataT>
struct member_function_job_copy_data
{
	typedef C ClassType;
	typedef traits::MemberFunction<C, void(JobSystem&, size_t, Job*, void*)> MemberFunction;
	typedef typename MemberFunction::Pointer MemberFunctionPtr;
	static const size_t PAD_SIZE = 0x30;

	member_function_job_copy_data(ClassType* pInst, MemberFunctionPtr function, const DataT& jobData) :
		pInst_(pInst),
		pFunction_(function)
	{
		memcpy(data_, &jobData, sizeof(DataT));
	}

	ClassType* pInst_;
	MemberFunctionPtr pFunction_;
	uint8_t data_[PAD_SIZE];
};


class JobSystem
{
	struct ThreadJobAllocator;


public:
	static const uint32_t AUTO_THREAD_COUNT = 0;
	static const uint32_t HW_THREAD_MAX = core::Min(1 << Job::THREAD_IDX_BITS, 12); // max even if hardware supports more.
	static const uint32_t HW_THREAD_NUM_DELTA = 1; // num = Min(max,hw_num-delta);
	static const size_t MAX_JOBS = 1 << Job::JOB_IDX_BITS;
	// 68kb(72kb x64) per 1024 jobs per thread
	// so for 4096 jobs with 6 threads it's: 1728kb ~1.7mb
	static const size_t MEMORY_PER_THREAD = (MAX_JOBS * sizeof(Job)) + (MAX_JOBS * sizeof(Job*));

	static_assert(ThreadQue::MAX_NUMBER_OF_JOBS == MAX_JOBS, "ThreadQue max jobs is not equal");

	typedef core::FixedArray<uint32_t, HW_THREAD_MAX > ThreadIdArray;

#if X_ENABLE_JOBSYS_PROFILER
	typedef std::array<JobSystemStats, JOBSYS_HISTORY_COUNT> ProfilerStatsArr;
	typedef std::array<JobQueueHistory*, HW_THREAD_MAX> ProfilerThreadTimelinesArr;
#endif // !X_ENABLE_JOBSYS_PROFILER

private:
	typedef core::FixedArray<std::pair<uint32_t, size_t>, HW_THREAD_MAX > ThreadIdToIndex;

	typedef core::MemoryArena<
		core::PoolAllocator,
		core::SingleThreadPolicy,
		core::NoBoundsChecking,
		core::NoMemoryTracking,
		core::NoMemoryTagging
	> JobArena;

public:
	JobSystem(core::MemoryArenaBase* arena);
	~JobSystem();

	bool StartUp(uint32_t threadCount = AUTO_THREAD_COUNT);
	void ShutDown(void);
	void OnFrameBegin(bool isProfilerPaused);

	void CreateQueForCurrentThread(void);
	uint32_t GetThreadCount(void) const;
	uint32_t GetQueueCount(void) const;
	ThreadIdArray getThreadIds(void);

#if X_ENABLE_JOBSYS_PROFILER
	X_INLINE int32_t getCurrentProfilerIdx(void) const;

	X_INLINE const ProfilerThreadTimelinesArr& GetTimeLines(void) const;
	X_INLINE const ProfilerStatsArr& GetStats(void) const;
#endif // !X_ENABLE_JOBSYS_PROFILER

private:
	bool StartThreads(void);
	void CreateThreadObjects(uint32_t threadId);


public:
	Job* CreateJob(JobFunction::Pointer function JOB_SYS_SUB_PARAM);
	Job* CreateJobAsChild(Job* pParent, JobFunction::Pointer function JOB_SYS_SUB_PARAM);

	X_INLINE Job* CreateJob(JobFunction::Pointer function, void* pData JOB_SYS_SUB_PARAM);
	X_INLINE Job* CreateJobAsChild(Job* pParent, JobFunction::Pointer function, void* pData JOB_SYS_SUB_PARAM);

	template<typename DataT, typename = std::enable_if_t<!std::is_pointer<DataT>::value>>
	X_INLINE Job* CreateJob(JobFunction::Pointer function, const DataT& data JOB_SYS_SUB_PARAM);
	template<typename DataT, typename = std::enable_if_t<!std::is_pointer<DataT>::value>>
	X_INLINE Job* CreateJobAsChild(Job* pParent, JobFunction::Pointer function, const DataT& data JOB_SYS_SUB_PARAM);

	template <typename T, typename SplitterT>
	X_INLINE Job* parallel_for(T* data, size_t count, 
		typename parallel_for_job_data<T,SplitterT>::DataJobFunctionPtr function, const SplitterT& splitter JOB_SYS_SUB_PARAM);

	template <typename T, typename SplitterT>
	X_INLINE Job* parallel_for_child(Job* pParent, T* data, size_t count,
		typename parallel_for_job_data<T, SplitterT>::DataJobFunctionPtr function, const SplitterT& splitter JOB_SYS_SUB_PARAM);


	template <typename ClassType, typename T, typename SplitterT>
	X_INLINE Job* parallel_for_member(typename parallel_for_member_job_data<ClassType, T, SplitterT>::FunctionDelagte del, 
		T* data, uint32_t count, const SplitterT& splitter JOB_SYS_SUB_PARAM);

	template <typename ClassType, typename T, typename SplitterT>
	X_INLINE Job* parallel_for_member_child(Job* pParent, typename parallel_for_member_job_data<ClassType, T, SplitterT>::FunctionDelagte del,
		T* data, uint32_t count, const SplitterT& splitter JOB_SYS_SUB_PARAM);

	template<typename ClassType>
	X_INLINE Job* CreateMemberJob(ClassType* pInst, typename member_function_job_data<ClassType>::MemberFunctionPtr pFunction,
		void* pJobData JOB_SYS_SUB_PARAM);

	template<typename ClassType, typename DataT, typename = std::enable_if_t<!std::is_pointer<DataT>::value>>
	X_INLINE Job* CreateMemberJob(ClassType* pInst, typename member_function_job_copy_data<ClassType, DataT>::MemberFunctionPtr pFunction,
		DataT& jobData JOB_SYS_SUB_PARAM);

	template<typename ClassType>
	X_INLINE Job* CreateMemberJobAndRun(ClassType* pInst, typename member_function_job_data<ClassType>::MemberFunctionPtr pFunction, 
		void* pJobData JOB_SYS_SUB_PARAM);

	template<typename ClassType, typename DataT, typename = std::enable_if_t<!std::is_pointer<DataT>::value>>
	X_INLINE Job* CreateMemberJobAndRun(ClassType* pInst, typename member_function_job_copy_data<ClassType, DataT>::MemberFunctionPtr pFunction,
		DataT& jobData JOB_SYS_SUB_PARAM);


	template<typename ClassType>
	X_INLINE Job* CreateMemberJobAsChild(Job* pParent, ClassType* pInst, 
		typename member_function_job_data<ClassType>::MemberFunctionPtr pFunction, void* pJobData JOB_SYS_SUB_PARAM);

	template<typename ClassType, typename DataT, typename = std::enable_if_t<!std::is_pointer<DataT>::value>>
	X_INLINE Job* CreateMemberJobAsChild(Job* pParent, ClassType* pInst, 
		typename member_function_job_copy_data<ClassType,DataT>::MemberFunctionPtr pFunction, DataT& jobData JOB_SYS_SUB_PARAM);

	X_INLINE void AddContinuation(Job* ancestor, Job* continuation, bool runInline = false);

	// empty job used for sync.
	X_INLINE static void EmptyJob(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);

	X_INLINE Job* CreateEmtpyJob(JOB_SYS_SUB_PARAM_SINGLE);
	X_INLINE Job* CreateEmtpyJobAsChild(Job* pPaerent JOB_SYS_SUB_PARAM);


	void Run(Job* pJob);
	X_INLINE void RunAndWait(Job* pJob);
	void Wait(Job* pJob);
	void WaitWithoutHelp(Job* pJob) const;
	bool HelpWithWork(void); // must be a job thread / main thread that calls this, return true if ran a job.


private:
	Job* AllocateJob(void);
	Job* AllocateJob(size_t threadIdx);
	void FreeJob(Job* pJob);

	bool CurrentThreadHasWorkerQueue(void) const;
	ThreadQue* GetWorkerThreadQueue(void) const;
	ThreadQue* GetWorkerThreadQueue(size_t threadIdx) const;
	ThreadJobAllocator* GetWorkerThreadAllocator(void) const;
	ThreadJobAllocator* GetWorkerThreadAllocator(size_t threadIdx) const;

	X_INLINE bool IsEmptyJob(Job* pJob) const;
	X_INLINE bool HasJobCompleted(Job* pJob) const;

	Job* GetJob(void);
	Job* GetJob(ThreadQue& queue);
	Job* GetJobCheckAllQues(ThreadQue& queue);
	void Execute(Job* pJob, size_t theadIdx);
	void Finish(Job* pJob, size_t threadIdx);

	size_t GetThreadIndex(void) const;
	bool CurrentThreadHasIndex(void) const;

	void ThreadBackOff(int32_t backoff);
	static Thread::ReturnValue ThreadRun_s(const Thread& thread);
	Thread::ReturnValue ThreadRun(const Thread& thread);

private:
	core::MemoryArenaBase* arena_;
	Thread threads_[HW_THREAD_MAX];
	ThreadIdToIndex threadIdToIndex_;
	uint32_t numThreads_;
	uint32_t numQueue_;

	core::ConditionVariable cond_;
	core::CriticalSection condCS_;

	ThreadQue* pThreadQues_[HW_THREAD_MAX];

private:
	X_ALIGNED_SYMBOL(struct ThreadJobAllocator, 64)
	{
		ThreadJobAllocator();

		uint32_t allocated;

		// there should be 60 byes of padding.
		Job jobs[JobSystem::MAX_JOBS];
	};


	ThreadJobAllocator* pJobAllocators_[HW_THREAD_MAX];

	ThreadLocalStorage ThreadQue_;
	ThreadLocalStorage ThreadAllocator_;

#if X_ENABLE_JOBSYS_PROFILER
	ProfilerThreadTimelinesArr pTimeLines_;

	core::AtomicInt currentHistoryIdx_;
	ProfilerStatsArr stats_;
#endif // !X_ENABLE_JOBSYS_PROFILER
};

X_ENABLE_WARNING(4324)

} // namespace V2

X_NAMESPACE_END

#include "JobSystem2.inl"
