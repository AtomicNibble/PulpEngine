#include "stdafx.h"
#include "Scheduler.h"


#include "Cpu.h"
#include "Core.h"

X_NAMESPACE_BEGIN(core)


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
	numCores = core::Max(core::Min(HW_THREAD_MAX, numCores - HW_THREAD_NUM_DELTA),1u);

	X_LOG0("Scheduler", "Creating %i threads", numThreads_);




}

void Scheduler::ShutDown(void)
{

}

void Scheduler::addJob(SJob& job)
{


}



X_NAMESPACE_END