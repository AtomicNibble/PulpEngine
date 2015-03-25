#pragma once

#ifndef X_SCHEDULER_H_
#define X_SCHEDULER_H_

#include "Time\TimeVal.h"

#include "Containers\Array.h"
#include "Containers\FixedRingBuffer.h"
#include "Containers\FixedFifo.h"

#include "Threading\Thread.h"
#include "Threading\Signal.h"
#include "Threading\AtomicInt.h"
#include "Threading\CriticalSection.h"


X_NAMESPACE_BEGIN(core)


typedef void(*Job)(void* pParam, uint32_t batchOffset, uint32_t batchNum, uint32_t workerIdx);


X_DECLARE_ENUM(JobListPriority)(NONE,LOW,NORMAL,HIGH);

static const uint32_t HW_THREAD_MAX = 6; // max even if hardware supports more.
static const uint32_t HW_THREAD_NUM_DELTA = 1; // num = Min(max,hw_num-delta);
static const uint32_t MAX_JOB_LISTS = 64;

class JobList;

struct ThreadStats
{
	ThreadStats() : numExecJobs(0) {}

	uint64_t numExecJobs;			// jobs execuced
	TimeVal startTime;				// time lists was first picked
	TimeVal waitTime;					// time spent waiting
	TimeVal threadExecTime;		// time spent executing jobs
	TimeVal threadTotalTime;	// total time.
};


struct JobListStats
{
	JobListStats() {
	}

	TimeVal submitTime;				// time lists was submitted
	TimeVal startTime;				// time lists was first picked
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
};


struct JobListThreadState
{
	JobListThreadState() :
	jobList(nullptr),
	lastJobIndex(0),
	nextJobIndex(-1) {}

	JobList* jobList;
	int	lastJobIndex;
	int	nextJobIndex;
};


class JobList
{
public:
	X_DECLARE_FLAGS(RunFlag)(OK,PROGRESS,DONE,STALLED);
	typedef Flags<RunFlag> RunFlags;

public:
	JobList();

	void AddJob(Job job, void* pData);

	void Wait(void);
	bool TryWait(void);
	bool IsSubmitted(void) const;
	bool IsDone(void) const;

	void SetPriority(JobListPriority::Enum priority);
	JobListPriority::Enum getPriority(void) const;


	RunFlags RunJobs(uint32_t threadIdx, JobListThreadState& state);

private:
	RunFlags RunJobsInternal(uint32_t threadIdx, JobListThreadState& state);

private:
	bool isDone_;
	bool isSubmit_;
	bool _pad[2];

	JobListPriority::Enum priority_;

	core::Array<JobData> jobs_;
	core::AtomicInt currentJob_;
	core::AtomicInt fetchLock_;
	core::AtomicInt numThreadsExecuting_;
	
	JobListStats stats_;
};


class JobThread : public ThreadAbstract
{
public:
	JobThread();
	~JobThread();

	void setThreadIdx(uint32_t idx);
	void AddJobList(JobList* pJobList);

	void SignalWork(void);

protected:
	virtual Thread::ReturnValue ThreadRun(const Thread& thread) X_FINAL;
	Thread::ReturnValue ThreadRunInternal(const Thread& thread);

private:
	core::Signal signalMoreWorkToDo_;
	core::CriticalSection signalCritical_;
	volatile bool moreWorkToDo_;


	ThreadStats stats_;

	core::FixedArray<JobList*, 32> jobLists_;

	uint32_t firstJobList_;			
	uint32_t lastJobList_;
	uint32_t threadIdx_;
};


class Scheduler
{
public:
	Scheduler();
	~Scheduler();

	void StartThreads(void);
	void ShutDown(void);

	void SubmitJobList(JobList* pList, JobList* pWaitFor = nullptr);

private:
	uint32_t numThreads_; // num created.
	JobThread threads_[HW_THREAD_MAX];

	core::FixedRingBuffer<JobList, 32> jobLists_;
	core::CriticalSection addJobListCrit_;
};


X_NAMESPACE_END

#endif // !X_SCHEDULER_H_
