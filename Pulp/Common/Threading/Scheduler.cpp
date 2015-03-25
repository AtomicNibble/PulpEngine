#include <EngineCommon.h>
#include "Scheduler.h"

#include "String\StackString.h"
#include "Util\Cpu.h"

#include "ICore.h"

// #include "SystemTimer.h"

X_NAMESPACE_BEGIN(core)


JobList::JobList() :
	isDone_(false),
	isSubmit_(false),
	priority_(JobListPriority::NORMAL),
	jobs_(gEnv->pArena),
	currentJob_(0),
	fetchLock_(0),
	numThreadsExecuting_(0)
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
		while (numThreadsExecuting_ > 0) {
			SwitchToThread();
		}
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

JobListPriority::Enum JobList::getPriority(void) const
{
	return priority_;
}


JobList::RunFlags JobList::RunJobs(uint32_t threadIdx, JobListThreadState& state)
{
//	TimeVal start = TimeVal();
	JobList::RunFlag res;

	++numThreadsExecuting_;

	res = RunJobsInternal(threadIdx, state);

	--numThreadsExecuting_;

//	stats_.threadTotalTime[threadIdx] += (TimeVal() - start);

	return RunFlag::OK;
}

JobList::RunFlags JobList::RunJobsInternal(uint32_t threadIdx, JobListThreadState& state)
{

	if ((++fetchLock_) == 1)
	{
		// grab a new job
		state.nextJobIndex = (++currentJob_) - 1;

		--fetchLock_;
	}
	else
	{
		--fetchLock_;
		return RunFlag::STALLED;
	}

	if (state.nextJobIndex >= jobs_.size()) {
		return RunFlag::DONE;
	}

	// execute the next job
	{
		JobData& job = jobs_[state.nextJobIndex];

//		TimeVal jobStart = 0;

		job.pJobRun(job.pData, 
			job.batchOffset, 
			job.batchNum,
			threadIdx);

		job.done = true;

//		TimeVal jobEnd = 0;
	//	stats_.threadExecTime[threadIdx] += jobEnd - jobStart;

	}

	if ((state.nextJobIndex+1) == jobs_.size()) {
		return RunFlag::DONE;
	}

	return RunFlag::OK;
}

// ----------------------------------


JobThread::JobThread()
{
	moreWorkToDo_ = false;

	firstJobList_ = 0;
	lastJobList_ = 0;
	threadIdx_ = 0;
}

JobThread::~JobThread()
{

}

void JobThread::setThreadIdx(uint32_t idx)
{
	threadIdx_ = idx;
}

void JobThread::AddJobList(JobList* pJobList)
{
	X_ASSERT_NOT_NULL(pJobList);
	jobLists_.append(pJobList);

	// tells the thread a joblist needs to be eaten.
	lastJobList_++;
}

void JobThread::SignalWork(void)
{
	core::CriticalSection::ScopedLock lock(signalCritical_);
	moreWorkToDo_ = true;
	signalMoreWorkToDo_.raise();
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
				signalMoreWorkToDo_.clear();
				signalCritical_.Leave();
			}
			else
			{
				signalCritical_.Leave();
				signalMoreWorkToDo_.wait();
			}
		}

		if (!thread.ShouldRun()) {
			break;
		}

		retVal = ThreadRunInternal(thread);
	}
	return retVal;
}

Thread::ReturnValue JobThread::ThreadRunInternal(const Thread& thread)
{
	X_UNUSED(thread);
	typedef core::FixedFifo<JobListThreadState, MAX_JOB_LISTS> JobStateFiFo;

	JobStateFiFo jobStates;

	while (thread.ShouldRun())
	{
		// can we fit any more jobs in the local stack.
		if (jobStates.size() < MAX_JOB_LISTS)
		{
			// if this is above zero we have one or more job lists waiting.
			if (firstJobList_ < lastJobList_)
			{
				JobListThreadState state;

				state.jobList = jobLists_[firstJobList_ & (MAX_JOB_LISTS - 1)];

				jobStates.push(state);
			}
		}

		if (jobStates.size() == 0) {
			break;
		}

		JobListThreadState& currentJobList = jobStates.peek();

		currentJobList.jobList->RunJobs(threadIdx_, currentJobList);


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
	ICore* pCore = (ICore*)gEnv->pCore;
	CpuInfo* pCpu = pCore->GetCPUInfo();

	uint32_t numCores = pCpu->GetCoreCount();
	numThreads_ = core::Max(core::Min(HW_THREAD_MAX, numCores - HW_THREAD_NUM_DELTA), 1u);

	X_LOG0("Scheduler", "Creating %i threads", numThreads_);

	uint32_t i;
	for (i = 0; i < numThreads_; i++)
	{
		core::StackString<64> name;
		name.appendFmt("Worker_%i", i);
		threads_[i].setThreadIdx(i);
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


void Scheduler::SubmitJobList(JobList* pList, JobList* pWaitFor)
{
	X_ASSERT_NOT_NULL(pList);

	core::CriticalSection::ScopedLock lock(addJobListCrit_);

	for (uint32_t i = 0; i < numThreads_; i++) {
		threads_[i].AddJobList(pList);
		threads_[i].SignalWork();
	}
}


X_NAMESPACE_END