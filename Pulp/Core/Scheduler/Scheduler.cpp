#include "stdafx.h"
#include "Scheduler.h"


#include "Cpu.h"
#include "Core.h"

X_NAMESPACE_BEGIN(core)


JobList::JobList() :
	isDone_(false),
	isSubmit_(false),
	jobs_(g_coreArena),
	currentJob_(0),
	fetchLock_(0)
{

}

void JobList::AddJob(Job job, void* pData)
{
	JobData data;
	data.pJobRun = job;
	data.pData = pData;
	data.batchOffset = 0;
	data.batchNum = 1;

	jobs_.append(data);
}

void JobList::Wait(void)
{
	if (jobs_.isNotEmpty()) 
	{
	
	}
	isDone_ = true;
}

bool JobList::TryWait(void)
{
	if (jobs_.isEmpty()) {
		Wait();
		return true;
	}
	return false;

}

bool JobList::IsSubmitted(void) const
{
	return isSubmit_;
}

bool JobList::IsDone(void) const
{
	return isDone_;
}

void JobList::SetPriority(JobListPriority::Enum priority)
{
	priority_ = priority;
}


// ----------------------------------


JobThread::JobThread()
{

}

JobThread::~JobThread()
{

}

Thread::ReturnValue JobThread::ThreadRun(const Thread& thread)
{
	X_UNUSED(thread);

	while (thread.ShouldRun())
	{
			// check for any new job lists that need to be copy to local thread stack.


			// go throught all the lcoal lists and find one with highest priority.



			// see if we should run one or multipe jobs in the context of this thread.


			// check the result of the job that was run.




	}

	return Thread::ReturnValue(0);
}


// ----------------------------------

Scheduler::Scheduler() :
numThreads_(0)
{

}


Scheduler::~Scheduler()
{

}



void Scheduler::StartThreads(void)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pCore);

	// get the num HW threads
	XCore* pCore = (XCore*)gEnv->pCore;
	CpuInfo* pCpu = pCore->GetCPUInfo();

	uint32_t numCores = pCpu->GetLogicalProcessorCount();
	numThreads_ = core::Max(core::Min(HW_THREAD_MAX, numCores - HW_THREAD_NUM_DELTA), 1u);

	X_LOG0("Scheduler", "Creating %i threads", numThreads_);

	uint32_t i;
	for (i = 0; i < numThreads_; i++)
	{
		core::StackString<64> name;
		name.appendFmt("Worker_%i", i);
		threads_[i].Create(name.c_str()); // default stack size.
		threads_[i].Start();
	}

}

void Scheduler::ShutDown(void)
{
	uint32_t i;

	for (i = 0; i < numThreads_; i++) {
		threads_[i].Stop(); 
	}
	for (i = 0; i < numThreads_; i++) {
		threads_[i].Join();
	}
}


void Scheduler::SubMitList(JobList* pList, JobList* pWaitFor)
{
	X_ASSERT_NOT_NULL(pList);

}


X_NAMESPACE_END