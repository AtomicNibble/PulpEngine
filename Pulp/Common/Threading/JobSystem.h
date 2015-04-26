#pragma once


#ifndef X_THREAD_JOB_SYS_H_
#define X_THREAD_JOB_SYS_H_

#include "IJobSystem.h"


#include "Containers\Array.h"
#include "Containers\FixedRingBuffer.h"
#include "Containers\Fifo.h"

#include "Threading\Thread.h"
#include "Threading\Signal.h"
#include "Threading\AtomicInt.h"
#include "Threading\CriticalSection.h"


X_NAMESPACE_BEGIN(core)


static const uint32_t HW_THREAD_MAX = 6; // max even if hardware supports more.
static const uint32_t HW_THREAD_NUM_DELTA = 1; // num = Min(max,hw_num-delta);


struct ThreadStats
{
	ThreadStats() : numExecLists(0) {}

	TimeVal waitforJobTime;
	uint64_t numExecLists;	// jobs lists execuced
};

class JobQue
{
public:
	JobQue();
	~JobQue();

	void setArena(core::MemoryArenaBase* arena);

	void AddJob(const JobDecl job);
	void AddJobs(JobDecl* pJobs, size_t numJobs);
	bool tryPop(JobDecl& job);

	size_t numJobs(void) const;

private:
	core::Spinlock lock_;
	core::Fifo<JobDecl> jobs_;
};


class JobThread : public ThreadAbstract
{
public:
	JobThread();
	~JobThread();

	void init(uint32_t idx, JobQue* ques);

	void SignalWork(void);
	void Stop(void);
	void WaitForThread(void);

private:
	TimeVal GetTimeReal(void) const;

protected:
	virtual Thread::ReturnValue ThreadRun(const Thread& thread) X_FINAL;
	Thread::ReturnValue ThreadRunInternal(const Thread& thread);

private:
	core::Signal signalWorkerDone_;
	core::Signal signalMoreWorkToDo_;
	core::CriticalSection signalCritical_;
	volatile bool moreWorkToDo_;

	ThreadStats stats_;
	uint32_t threadIdx_;

	JobQue* ques_[JobPriority::ENUM_COUNT];
	core::ITimer* pTimer_;
};

class JobSystem : public IJobSystem
{
public:
	JobSystem();
	~JobSystem() X_FINAL;

	bool StartUp(void) X_FINAL;
	void ShutDown(void) X_FINAL;

	void AddJob(const JobDecl job, JobPriority::Enum priority = JobPriority::NORMAL) X_FINAL;
	void AddJobs(JobDecl* pJobs, size_t numJobs, JobPriority::Enum priority = JobPriority::NORMAL) X_FINAL;

	void waitForAllJobs(void) X_FINAL;

	int32_t numThreads(void) const X_FINAL;
private:
	bool StartThreads(void);

private:
	int32_t numThreads_; // num created.
	JobThread threads_[HW_THREAD_MAX];
	JobQue ques_[JobPriority::ENUM_COUNT];
public:
	static int var_LongJobMs;
};




X_NAMESPACE_END

#endif // !X_THREAD_JOB_SYS_H_