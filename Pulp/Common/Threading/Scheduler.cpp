#include <EngineCommon.h>
#include "Scheduler.h"

#include "String\StackString.h"
#include "Util\Cpu.h"

#include "ICore.h"
#include "ITimer.h"

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

void JobList::Clear(void)
{
	isDone_ = false;
	isSubmit_ = false;
	syncCount_ = 0;
	currentJob_ = 0;
	fetchLock_ = 0;
	numThreadsExecuting_ = 0;
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
#if 0
		// check it was submitted
		if(!IsSubmitted()) {
			X_WARNING("JobList", "wait was called on a list that was never submitted");
			return;
		}
#endif 

		TimeVal waitStart = GetTimeReal();
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
			stats_.waitTime = TimeVal(0ll);
		}

		Clear();
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
	JobList::RunFlags res;

	++numThreadsExecuting_;

	res = RunJobsInternal(threadIdx, state);

	--numThreadsExecuting_;

	stats_.threadTotalTime[threadIdx] += (GetTimeReal() - start);

	return res;
}

JobList::RunFlags JobList::RunJobsInternal(uint32_t threadIdx, JobListThreadState& state)
{
	if (stats_.startTime.GetValue() == 0) {
		stats_.startTime = GetTimeReal();
	}

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
		stats_.endTime = GetTimeReal();
		return RunFlag::DONE;
	}

	return RunFlag::OK;
}

void JobList::PreSubmit(void)
{
	stats_.submitTime = GetTimeReal();
	syncCount_ = safe_static_cast<int32_t, size_t>(jobs_.size());
}

TimeVal JobList::GetTimeReal(void) const
{
	return pTimer_->GetTimeReal();
}
// ----------------------------------


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

void JobThread::AddJobList(JobList* pJobList)
{
	X_ASSERT_NOT_NULL(pJobList);

	while (jobLists_.size() == jobLists_.capacity()) {
		SwitchToThread();
	}

	jobLists_.push(pJobList);
}

void JobThread::SignalWork(void)
{
	core::CriticalSection::ScopedLock lock(signalCritical_);
	moreWorkToDo_ = true;
	signalMoreWorkToDo_.raise();
}

void JobThread::Stop(void)
{
	ThreadAbstract::Stop();
	SignalWork();
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
	static const size_t InvalidJobIdx = static_cast<size_t>(-1);

	JobStateList jobStates;
	JobList::RunFlags jobResult;
	JobListPriority::Enum priority;
	size_t currentJobListIdx;
	size_t lastStalledJobList = InvalidJobIdx;

	while (thread.ShouldRun())
	{
		// can we fit any more jobs in the local stack.
		if (jobStates.size() < jobStates.capacity())
		{
			// any to add?
			if (jobLists_.IsNotEmpty())
			{
				JobListThreadState state;

				state.jobList = jobLists_.peek();

				jobStates.append(state);
				jobLists_.pop();
			}
		}

		if (jobStates.isEmpty()) {
			break;
		}

		currentJobListIdx = 0;
		priority = JobListPriority::NONE;

		if (lastStalledJobList == InvalidJobIdx)
		{
			// pick a job list with the most priority.
			for (size_t i = 0; i < jobStates.size(); i++) {
				if (jobStates[i].jobList->getPriority() > priority) {
					priority = jobStates[i].jobList->getPriority();
					currentJobListIdx = i;
				}
			}
		}
		else
		{
			// find a job with equal or higher priorty as the stall.
			JobListThreadState& stalledList = jobStates[lastStalledJobList];
			priority = stalledList.jobList->getPriority();
			currentJobListIdx = lastStalledJobList;

			for (size_t i = 0; i < jobStates.size(); i++) {
				if (i != lastStalledJobList && jobStates[i].jobList->getPriority() > priority) {
					priority = jobStates[i].jobList->getPriority();
					currentJobListIdx = i;
				}
			}
		}

		JobListThreadState& currentJobList = jobStates[currentJobListIdx];

		jobResult = currentJobList.jobList->RunJobs(threadIdx_, currentJobList);

		if (jobResult.IsSet(JobList::RunFlag::STALLED))
		{
			// we stalled :()
			lastStalledJobList = currentJobListIdx;		
		}
		if (jobResult.IsSet(JobList::RunFlag::DONE))
		{
			// no more work for me!
			jobStates.removeIndex(currentJobListIdx);
			lastStalledJobList = InvalidJobIdx; // reset stall idx.

			stats_.numExecLists++;
		}		
		else
		{
			lastStalledJobList = InvalidJobIdx;
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
	X_LOG0("Scheduler", "Shuting down");

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