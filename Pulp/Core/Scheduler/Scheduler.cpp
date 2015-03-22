#include "stdafx.h"
#include "Scheduler.h"


#include "Cpu.h"
#include "Core.h"

X_NAMESPACE_BEGIN(core)

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




X_NAMESPACE_END