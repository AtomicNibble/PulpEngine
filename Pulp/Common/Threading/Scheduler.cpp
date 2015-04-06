#include <EngineCommon.h>
#include "Scheduler.h"

#include "String\StackString.h"
#include "Util\Cpu.h"

#include "ICore.h"
#include "ITimer.h"
#include "IConsole.h"

// #include "SystemTimer.h"

X_NAMESPACE_BEGIN(core)

int32_t JobList::JOB_LIST_DONE = 0x12345678;

int Scheduler::var_LongJobMs = 8;

void JobList::NopJob(void* pParam, uint32_t batchOffset,
	uint32_t batchNum, uint32_t workerIdx)
{
#if SCHEDULER_LOGS
	X_LOG0("NopJob", "NobJob: pParam: %x workerIdx: %x", pParam, workerIdx);
#endif // !SCHEDULER_LOGS
}


JobList::JobList(core::MemoryArenaBase* arena) :
	isDone_(false),
	isSubmit_(false),
	priority_(JobListPriority::NORMAL),
	jobs_(arena),
	syncCount_(0),
	currentJob_(0),
	fetchLock_(0),
	numThreadsExecuting_(0),
	version_(0),

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
	data.done = false;
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
			core::Thread::Yield();
			waited = true;
		}

		++version_;

		while (numThreadsExecuting_ > 0) {
			core::Thread::Yield();
			waited = true;
		}

		if(waited) {
			stats_.waitTime = GetTimeReal() - waitStart;
		} else {
			// if some goat calls wait twice it will set it to zero.
			// if first time it's already goingto be zero.
			stats_.waitTime = TimeVal(0ll);
		}

		jobs_.clear();
	}
#if SCHEDULER_LOGS
	X_LOG0("JobList", "is done id: %x", listId_);
#endif // !SCHEDULER_LOGS
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


JobList::RunFlags JobList::RunJobs(uint32_t threadIdx, JobListThreadState& state, bool singleJob)
{
	TimeVal start = GetTimeReal();
	JobList::RunFlags res;

	++numThreadsExecuting_;

	res = RunJobsInternal(threadIdx, state, singleJob);

	stats_.threadTotalTime[threadIdx] += (GetTimeReal() - start);

	// do it after we got the time.
	--numThreadsExecuting_;

	return res;
}

JobList::RunFlags JobList::RunJobsInternal(uint32_t threadIdx, JobListThreadState& state, bool singleJob)
{
	if (state.version != version_) {
		// trying to run an old version of this list that is already done
		//	X_LOG0("Joblist", "old version. sate: %i thisVersion: %i currentJob: %i", 
		//		state.version, version_, currentJob_);
		return RunFlag::DONE;
	}

	if (stats_.startTime.GetValue() == 0) {
		stats_.startTime = GetTimeReal();
	}

	RunFlags resFalgs = RunFlag::OK;

	do 
	{
		// look at all the jobs below the current job.
		// if it's the job list done job.
		for (; state.lastJobIndex < currentJob_
			&& state.lastJobIndex < jobs_.size(); state.lastJobIndex++)
		{
			if (jobs_[state.lastJobIndex].pData == &JOB_LIST_DONE)
			{
				if (syncCount_ > 0) {
					return resFalgs | RunFlag::STALLED;
				}
			}
		}

		{
			if ((++fetchLock_) == 1)
			{
				// grab a new job
				state.nextJobIndex = (++currentJob_) - 1;

				// run through any remaining signals and syncs (this should rarely iterate more than once)
				for (; state.lastJobIndex <= state.nextJobIndex &&
					state.lastJobIndex < jobs_.size(); state.lastJobIndex++) 
				{
					if (jobs_[state.lastJobIndex].pData == &JOB_LIST_DONE)
					{
						if (syncCount_ > 0) 
						{
							// return this job to the list
							--currentJob_;
							// release the fetch lock
							--fetchLock_;
							// stalled on a synchronization point
							return resFalgs | RunFlag::STALLED;
						}
					}
				}

				--fetchLock_;
			}
			else
			{
				--fetchLock_;
				return resFalgs | RunFlag::STALLED;
			}

		}

		// end of the job lists?
		if (state.nextJobIndex >= jobs_.size()) {
			return resFalgs | RunFlag::DONE;
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
			TimeVal elapsed = jobEnd - jobStart;

			job.execTime = elapsed;
			stats_.threadExecTime[threadIdx] += elapsed;

			if (Scheduler::var_LongJobMs > 0)
			{
				if (elapsed.GetMilliSeconds() > Scheduler::var_LongJobMs)
				{				
					X_WARNING("Scheduler", "a single job took more than: %gms elapsed: %gms "
						"pFunc: %p pData: %p batchOffset: %i batchNum: %i", 
						Scheduler::var_LongJobMs,
						elapsed.GetMilliSeconds(),
						job.pJobRun,
						job.pData,
						job.batchOffset,
						job.batchNum
						);
				}
			}
		}


		// we made progress
		resFalgs |= RunFlag::PROGRESS;

		--syncCount_;

		if (syncCount_ == 0) {
			stats_.endTime = GetTimeReal();
	#if SCHEDULER_LOGS
			X_LOG0("JobList", "sync count 0, for id: %x", listId_);
	#endif // !SCHEDULER_LOGS
			return resFalgs | RunFlag::DONE;
		}

	}
	while (!singleJob);

	return resFalgs;
}


void JobList::PreSubmit(void)
{
	currentJob_ = 0;
	syncCount_ = safe_static_cast<int32_t, size_t>(jobs_.size());

	JobData endJob;
	endJob.pJobRun = NopJob;
	endJob.pData = &JOB_LIST_DONE;
	endJob.done = false;
	endJob.batchOffset = 0;
	endJob.batchNum = 0;
	jobs_.append(endJob);


	stats_.submitTime = GetTimeReal();
}

void JobList::OnSubmited(void)
{
	isSubmit_ = true;
	isDone_ = false;
}


TimeVal JobList::GetTimeReal(void) const
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

	while (lastJobList_ - firstJobList_ >= MAX_JOB_LISTS) {
		core::Thread::Yield();
	}

	pJobList->OnSubmited();

	jobLists_[lastJobList_ & (MAX_JOB_LISTS - 1)].jobList = pJobList;
	jobLists_[lastJobList_ & (MAX_JOB_LISTS - 1)].version = pJobList->version_;
	lastJobList_++;
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
			if (firstJobList_ < lastJobList_) 
			{
				JobListThreadState state;

				state.jobList = jobLists_[firstJobList_ & (MAX_JOB_LISTS - 1)].jobList;
				state.version = jobLists_[firstJobList_ & (MAX_JOB_LISTS - 1)].version;

				firstJobList_++;

				jobStates.append(state);
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

		bool singleJob = (priority == JobListPriority::HIGH) ? false : true;

		jobResult = currentJobList.jobList->RunJobs(threadIdx_, currentJobList, singleJob);

		if (jobResult.IsSet(JobList::RunFlag::STALLED))
		{
			// we stalled :()
			lastStalledJobList = currentJobListIdx;		
		}
		if (jobResult.IsSet(JobList::RunFlag::DONE))
		{
			// no more work for me!
			jobStates.removeIndex(currentJobListIdx);
			// done with this job list so remove it from the local list

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

void Scheduler::StartUp(void)
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
	ADD_CVAR_REF("scheduler_numThreads", numThreads_, numThreads_, 1,
		HW_THREAD_MAX, core::VarFlag::SYSTEM,
		"The number of threads used by the job scheduler");

	// register longJob
	ADD_CVAR_REF("scheduler_longJobMs", var_LongJobMs, var_LongJobMs, 0, 32, core::VarFlag::SYSTEM, "If a single job takes longer than this a warning is printed.\n0 = disabled.");


	StartThreads();
}

void Scheduler::StartThreads(void)
{
	X_LOG0("Scheduler", "Creating %i threads", numThreads_);

	int32_t i;
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

	int32_t i;

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

	for (int32_t i = 0; i < numThreads_; i++) {
		threads_[i].AddJobList(pList);
		threads_[i].SignalWork();
	}
}

void Scheduler::WaitForThreads(void)
{
	for (int32_t i = 0; i < numThreads_; i++) {
		threads_[i].WaitForThread();
	}
}

int32_t Scheduler::numThreads(void) const
{
	return numThreads_;
}


X_NAMESPACE_END