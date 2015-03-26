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
	syncCount_(0),
	currentJob_(0),
	fetchLock_(0),
	numThreadsExecuting_(0),
	pTimer_(nullptr)
{
	// set the timer val.
	// job lists should only be created once the core has been setup.
	// menaing timer is set.
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pTimer);

	pTimer_ = gEnv->pTimer;
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
		// check it was submitted
		if(!IsSubmitted()) {
			X_WARNING("JobList", "wait was called on a list that was never submitted");
			return;
		}

		TimVal waitStart = GetTimeReal();
		bool waited = false;

		while (syncCount_ > 0) {
			SwitchToThread();
			waited = true;
		}

		while (numThreadsExecuting_ > 0) {
			SwitchToThread();
			waited = true;
		}

		if(waited) {
			stats_.waitTime = GetTimeReal() - waitStart;
		} else {
			// if some goat calls wait twice it will set it to zero.
			// if first time it's already goingto be zero.
			stats_.waitTime = 0;
		}
	}
	isDone_ = true;
}

bool JobList::TryWait(void)
{
	if (jobs_.isEmpty() || syncCount_ <= 0) {
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
  TimeVal start = GetTimeReal();
	JobList::RunFlag res;

	++numThreadsExecuting_;

	res = RunJobsInternal(threadIdx, state);

	--numThreadsExecuting_;

	stats_.threadTotalTime[threadIdx] += (GetTimeReal() - start);

	return res;
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

		TimeVal jobStart = GetTimeReal();

		job.pJobRun(job.pData, 
			job.batchOffset, 
			job.batchNum,
			threadIdx);

		job.done = true;

		TimeVal jobEnd = GetTimeReal();
		stats_.threadExecTime[threadIdx] += jobEnd - jobStart;

		--syncCount_;
	}

	if ((state.nextJobIndex+1) == jobs_.size()) {
		return RunFlag::DONE;
	}

	return RunFlag::OK;
}

void JobList::PreSubmit(void)
{
	syncCount_ = safe_static_cast<int32_t, size_t>(jobs_.size());
}

TimeVal JobList::(void) const
{
		return pTimer_->GetTimeReal();
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
	JobStateFiFo jobStates;
	JobList::RunFlags jobResult;

	while (thread.ShouldRun())
	{
		// can we fit any more jobs in the local stack.
		if (jobStates.size() < jobStates.capacity())
		{
			// if this is above zero we have one or more job lists waiting.
			if (firstJobList_ < lastJobList_)
			{
				JobListThreadState state;

				state.jobList = jobLists_[firstJobList_ & (jobStates.capacity() - 1)];

				jobStates.push(state);
			}
		}

		if (jobStates.size() == 0) {
			break;
		}

		// this need to pick a job lists with the most priority.
		// i think i will make a stack based container that is priority based,


		JobListThreadState& currentJobList = jobStates.peek();

		jobResult = currentJobList.jobList->RunJobs(threadIdx_, currentJobList);

		if(jobResult.isSet(RunFlag::STALLED))
		{
			// we stalled :()
			// try find another job list to work on with simular priority.
			
		}
		if(jobResult.isSet(RunFlag::DONE))
		{
			// no more work for me!

		}		
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

	pList->PreSubmit();

	for (uint32_t i = 0; i < numThreads_; i++) {
		threads_[i].AddJobList(pList);
		threads_[i].SignalWork();
	}
}


X_NAMESPACE_END