#pragma once


#ifndef X_THREAD_JOB_SYS_H_
#define X_THREAD_JOB_SYS_H_

#include "Time\TimeVal.h"

#include "Containers\Array.h"
#include "Containers\FixedRingBuffer.h"
#include "Containers\FixedFifo.h"

#include "Threading\Thread.h"
#include "Threading\Signal.h"
#include "Threading\AtomicInt.h"
#include "Threading\CriticalSection.h"


X_NAMESPACE_BEGIN(core)

typedef void(*Job)(void* pParam);

struct JobDecl
{
	Job pJobFunc;
	void* pParam;
};


X_DECLARE_ENUM(JobPriority)(NONE, LOW, NORMAL, HIGH);

static const uint32_t HW_THREAD_MAX = 6; // max even if hardware supports more.
static const uint32_t HW_THREAD_NUM_DELTA = 1; // num = Min(max,hw_num-delta);


struct ThreadStats
{
	ThreadStats() : numExecLists(0) {}

	TimeVal waitforJobTime;
	uint64_t numExecLists;	// jobs lists execuced
};


class JobThread : public ThreadAbstract
{
public:
	JobThread();
	~JobThread();

	void setThreadIdx(uint32_t idx);
	void SignalWork(void);
	void Stop(void);
	void WaitForThread(void);

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
};


class JobSystem
{
public:
	JobSystem();
	~JobSystem();

	bool StartUp(void);
	void ShutDown(void);

	void AddJob(const JobDecl job);

	int32_t numThreads(void) const;

private:
	bool StartThreads(void);

private:
	int32_t numThreads_; // num created.
	JobThread threads_[HW_THREAD_MAX];

	static int var_LongJobMs;
};




X_NAMESPACE_END

#endif // !X_THREAD_JOB_SYS_H_