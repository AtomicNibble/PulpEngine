#pragma once

#ifndef X_SCHEDULER_H_
#define X_SCHEDULER_H_

#include "Time\TimeVal.h"

#include "Containers\Array.h"
#include "Containers\FixedFifo.h"

#include "Threading\Thread.h"
#include "Threading\Signal.h"
#include "Threading\AtomicInt.h"
#include "Threading\CriticalSection.h"


X_NAMESPACE_BEGIN(core)

namespace JobList
{

#define SCHEDULER_LOGS 0

typedef void(*Job)(void* pParam, uint32_t batchOffset, uint32_t batchNum, uint32_t workerIdx);

X_DECLARE_ENUM(JobListPriority)(NONE,LOW,NORMAL,HIGH);

static const uint32_t HW_THREAD_MAX = 6; // max even if hardware supports more.
static const uint32_t HW_THREAD_NUM_DELTA = 1; // num = Min(max,hw_num-delta);
static const uint32_t MAX_JOB_LISTS = 64;

class JobList;
class jobListRunner;

struct ThreadStats
{
	ThreadStats() : numExecLists(0) {}

	TimeVal waitforJobTime;
	uint64_t numExecLists;	// jobs lists execuced
};


struct JobListStats
{
	TimeVal submitTime;					// time lists was submitted
	TimeVal startTime;					// time lists was first picked
	TimeVal waitTime;					// time list spent waiting
	TimeVal endTime;					// time at which all the jobs are done.
	// stats for each thread, since the jobs will run in multipl threads
	// this just gives a break down of how much time each thread spent.
	TimeVal threadExecTime[HW_THREAD_MAX];		// time exec job code
	TimeVal threadTotalTime[HW_THREAD_MAX];		// total time.
};



struct JobData
{
	Job pJobRun;
	void *pData;

	// done
	bool done;
	bool _pad[3];

	// batch offset and index.
	uint32_t batchOffset;
	uint32_t batchNum;

	TimeVal execTime;
};

static const size_t temp = sizeof(JobData);

struct JobListThreadState
{
	JobListThreadState() :
	jobList(nullptr),
	lastJobIndex(0),
	nextJobIndex(-1),
	version(0xFFFFFFFF) {}

	~JobListThreadState() {
		jobList = nullptr;
	}

	JobList* jobList;
	int	lastJobIndex;
	int	nextJobIndex;
	int version;
};


class JobList
{
public:
	X_DECLARE_FLAGS(RunFlag)(OK,PROGRESS,DONE,STALLED);
	typedef Flags<RunFlag> RunFlags;

	friend class jobListRunner;
	friend class JobThread;
public:
	explicit JobList(core::MemoryArenaBase* arena);

	void AddJob(Job job, void* pData);

	void Wait(void);
	bool TryWait(void);
	bool IsSubmitted(void) const;
	bool IsDone(void) const;

	void SetPriority(JobListPriority::Enum priority);
	JobListPriority::Enum getPriority(void) const;

	RunFlags RunJobs(uint32_t threadIdx, JobListThreadState& state, bool singleJob);

	const JobListStats& getStats(void) const {
		return stats_;
	}

	X_INLINE bool CanRun(void) const {
		if(waitForList_) {
			if(waitForList_ > 0 )
				return false;
		}
		return true;
	}

private:
	RunFlags RunJobsInternal(uint32_t threadIdx, JobListThreadState& state, bool singleJob);

protected:
	void PreSubmit(JobList* pWaitFor);

	TimeVal GetTimeReal(void) const;

private:
	static void NopJob(void* pParam, uint32_t batchOffset, uint32_t batchNum, uint32_t workerIdx);
	static int32_t JOB_LIST_DONE;

private:
	bool isDone_;
	bool isSubmit_;
	bool _pad[2];

	JobListPriority::Enum priority_;

	core::Array<JobData> jobs_;

	core::AtomicInt syncCount_;
	core::AtomicInt currentJob_;
	core::AtomicInt fetchLock_;
	core::AtomicInt numThreadsExecuting_;
	core::AtomicInt version_;

	core::AtomicInt doneGuard_;

	// points to a jobs done guard.
	core::AtomicInt* waitForList_;

	// keep a copy of the timer interface.
	core::ITimer* pTimer_;
	JobListStats stats_;

public:

	size_t listId_;
};


class JobThread : public ThreadAbstract
{
	typedef core::FixedArray<JobListThreadState, MAX_JOB_LISTS> JobStateList;

	struct threadJobList
	{
		threadJobList();

		JobList* jobList;
		int32_t	version;
	};


public:
	JobThread();
	~JobThread();

	void setThreadIdx(uint32_t idx);
	void AddJobList(JobList* pJobList);

	void SignalWork(void);
	void Stop(void);
	void WaitForThread(void);

protected:
	virtual Thread::ReturnValue ThreadRun(const Thread& thread) X_FINAL;
	Thread::ReturnValue ThreadRunInternal(const Thread& thread);

	TimeVal GetTimeReal(void) const;

private:
	core::Signal signalWorkerDone_;
	core::Signal signalMoreWorkToDo_;
	core::CriticalSection signalCritical_;
	volatile bool moreWorkToDo_;

	ThreadStats stats_;

//	core::FixedFifo<JobList*, MAX_JOB_LISTS> jobLists_;
	threadJobList jobLists_[MAX_JOB_LISTS];
	uint32_t firstJobList_;	
	uint32_t lastJobList_;

	uint32_t threadIdx_;
};


class jobListRunner
{
public:
	jobListRunner();
	~jobListRunner();

	void StartUp(void);
	void ShutDown(void);

	void SubmitJobList(JobList* pList, JobList* pWaitFor = nullptr);
	void WaitForThreads(void);

	int32_t numThreads(void) const;

private:
	void StartThreads(void);

private:
	int32_t numThreads_; // num created.
	JobThread threads_[HW_THREAD_MAX];

	core::CriticalSection addJobListCrit_;

public:
	static int32_t var_LongJobMs;
};

} // namespace JobList

X_NAMESPACE_END

#endif // !X_SCHEDULER_H_
