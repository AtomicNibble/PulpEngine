#pragma once

#ifndef X_SCHEDULER_H_
#define X_SCHEDULER_H_

#include "Time\TimeVal.h"

#include "Containers\Array.h"
#include "Containers\FixedRingBuffer.h"

#include "Threading\Thread.h"
#include "Threading\AtomicInt.h"
#include "Threading\CriticalSection.h"


X_NAMESPACE_BEGIN(core)


typedef void(*Job)(void* pParam);


X_DECLARE_ENUM(JobListPriority)(LOW,NORMAL,HIGH);

static const uint32_t HW_THREAD_MAX = 6; // max even if hardware supports more.
static const uint32_t HW_THREAD_NUM_DELTA = 1; // num = Min(max,hw_num-delta);
static const uint32_t MAX_JOB_LISTS = 64;


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

	// batch offset and index.
	uint32_t batchOffset;
	uint32_t batchNum;
};


class JobList
{
public:
	JobList();

	void AddJob(Job job, void* pData);

	void Wait(void);
	bool TryWait(void);
	bool IsSubmitted(void) const;
	bool IsDone(void) const;

	void SetPriority(JobListPriority::Enum priority);

private:
	bool isDone_;
	bool isSubmit_;
	bool _pad[2];

	JobListPriority::Enum priority_;

	core::Array<JobData> jobs_;
	core::AtomicInt currentJob_;
	core::AtomicInt fetchLock_;

	JobListStats stats_;
};



class JobThread : public ThreadAbstract
{
public:
	JobThread();
	~JobThread();

	virtual Thread::ReturnValue ThreadRun(const Thread& thread) X_FINAL;

private:
	ThreadStats stats_;
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
