#include <EngineCommon.h>
#include "JobSystem.h"

#include "String\StackString.h"
#include "Util\Cpu.h"

#include "ICore.h"
#include "ITimer.h"
#include "IConsole.h"


X_NAMESPACE_BEGIN(core)


int JobSystem::var_LongJobMs = 8;


JobThread::JobThread()
{
	moreWorkToDo_ = false;
	threadIdx_ = 0;
}

JobThread::~JobThread()
{

}

void JobThread::setThreadIdx(uint32_t idx)
{
	threadIdx_ = idx;
}

void JobThread::SignalWork(void)
{
	core::CriticalSection::ScopedLock lock(signalCritical_);
	moreWorkToDo_ = true;
	signalMoreWorkToDo_.raise();
	signalWorkerDone_.clear();
}

void JobThread::Stop(void)
{
	ThreadAbstract::Stop();
	SignalWork();
}

void JobThread::WaitForThread(void)
{
	signalWorkerDone_.wait();
}


Thread::ReturnValue JobThread::ThreadRun(const Thread& thread)
{
	Thread::ReturnValue retVal = Thread::ReturnValue(0);

	while (true)
	{
		{
			signalCritical_.Enter();

			if (moreWorkToDo_)
			{
				moreWorkToDo_ = false;
				signalMoreWorkToDo_.clear();
				signalCritical_.Leave();
			}
			else
			{
				signalWorkerDone_.raise();
				signalCritical_.Leave();

				TimeVal start = gEnv->pTimer->GetTimeReal();

				signalMoreWorkToDo_.wait();

				TimeVal end = gEnv->pTimer->GetTimeReal();

				stats_.waitforJobTime += (end - start);
				continue;
			}
		}

		if (!thread.ShouldRun()) {
			break;
		}

		retVal = ThreadRunInternal(thread);
	}

	signalWorkerDone_.raise();

	return retVal;
}

Thread::ReturnValue JobThread::ThreadRunInternal(const Thread& thread)
{
	while (thread.ShouldRun())
	{

	}
	return Thread::ReturnValue(0);
}

// ======================================

JobSystem::JobSystem() : numThreads_(0)
{

}

JobSystem::~JobSystem()
{

}



bool JobSystem::StartUp(void)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pCore);
	X_ASSERT_NOT_NULL(gEnv->pConsole);

	// get the num HW threads
	ICore* pCore = (ICore*)gEnv->pCore;
	CpuInfo* pCpu = pCore->GetCPUInfo();

	int32_t numCores = pCpu->GetCoreCount();
	numThreads_ = core::Max(core::Min(HW_THREAD_MAX, numCores - HW_THREAD_NUM_DELTA), 1u);

	// create a var.
	ADD_CVAR_REF("jobsys_numThreads", numThreads_, numThreads_, 1,
		HW_THREAD_MAX, core::VarFlag::SYSTEM,
		"The number of threads used by the job system");

	// register longJob
	ADD_CVAR_REF("jobsys_longJobMs", var_LongJobMs, var_LongJobMs, 0, 32, core::VarFlag::SYSTEM, "If a single job takes longer than this a warning is printed.\n0 = disabled.");

	return StartThreads();
}

void JobSystem::AddJob(const JobDecl job)
{


}


bool JobSystem::StartThreads(void)
{
	X_LOG0("JobSystem", "Creating %i threads", numThreads_);

	int32_t i;
	for (i = 0; i < numThreads_; i++)
	{
		core::StackString<64> name;
		name.appendFmt("Worker_%i", i);
		threads_[i].setThreadIdx(i);
		threads_[i].Create(name.c_str()); // default stack size.
		threads_[i].Start();
	}

	return true;
}

void JobSystem::ShutDown(void)
{
	X_LOG0("JobSystem", "Shuting down");

	int32_t i;

	for (i = 0; i < numThreads_; i++) {
		threads_[i].Stop();
	}
	for (i = 0; i < numThreads_; i++) {
		threads_[i].Join();
	}
}



X_NAMESPACE_END