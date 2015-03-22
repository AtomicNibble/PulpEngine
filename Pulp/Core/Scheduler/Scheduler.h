#pragma once

#ifndef X_SCHEDULER_H_
#define X_SCHEDULER_H_

#include "Time\TimeVal.h"

#include "Threading\Thread.h"

X_NAMESPACE_BEGIN(core)

/*

Ok this is going to be the threading job system.
it will take jobs that are a function and also take data.

maybe i will make it class based also.

the later is more the c++ way.


It should be possible to add loads of jobs then sync them.
So that any cost of adding the jobs can be done after all the required jobs have been added.

A job should be able to add other jobs.
And also wait for them to complte before it continues.


this is why fibers is probs a good solution since i can have a job running in a thread.
that adds jobs to the schedular then waits for them to finish before carrying on.
becuases of fibres the job that is waiting is not blocking any thead.

that thread is free todo other things in the meantime, preventing threads from becoming idle.

Which means i will need a pool of fibers that can be run within x number of threads.
i also need todo like load balancing so threads don't get starved.

how can i take my jobs and place them in fibres.
guess a fibere will have to get the job then run it in the context of the fibre.


*/


typedef void(*Job)(void* pParam);


X_DECLARE_ENUM(JobPriority)(LOW,NORMAL,HIGH);


struct ThreadStats
{
	ThreadStats() : numExecJobs(0) {}

	uint64_t numExecJobs;		// jobs execuced
	TimeVal waitTime;			// time spent waiting
	TimeVal threadExecTime;		// time spent executing jobs
	TimeVal threadTotalTime;	// total time.
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
	static const uint32_t HW_THREAD_MAX = 6; // max even if hardware supports more.
	static const uint32_t HW_THREAD_NUM_DELTA = 1; // num = Min(max,hw_num-delta);


public:
	Scheduler();
	~Scheduler();

	void StartThreads(void);
	void ShutDown(void);


private:
	uint32_t numThreads_; // num created.

	JobThread threads_[HW_THREAD_MAX];
};


X_NAMESPACE_END

#endif // !X_SCHEDULER_H_
