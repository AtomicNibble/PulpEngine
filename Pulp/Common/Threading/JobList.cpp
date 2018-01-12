#include <EngineCommon.h>
#include "JobList.h"

#include "Util\Cpu.h"
#include <Time\StopWatch.h>

#include "ICore.h"
#include "ITimer.h"
#include "IConsole.h"

// #include "SystemTimer.h"

X_NAMESPACE_BEGIN(core)

namespace JobList
{

int32_t JobList::JOB_LIST_DONE = 0x12345678;

int jobListRunner::var_LongJobMs = 8;

void JobList::NopJob(void* pParam, uint32_t batchOffset,
	uint32_t batchNum, uint32_t workerIdx)
{
	X_UNUSED(pParam);
	X_UNUSED(batchOffset);
	X_UNUSED(batchNum);
	X_UNUSED(workerIdx);

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
	doneGuard_(0),

	waitForList_(nullptr),

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

		core::StopWatch timer;
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
			stats_.waitTime = timer.GetTimeVal();
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
	core::StopWatch timer;
	JobList::RunFlags res;

	++numThreadsExecuting_;

	res = RunJobsInternal(threadIdx, state, singleJob);

	stats_.threadTotalTime[threadIdx] += timer.GetTimeVal();

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
			&& state.lastJobIndex < safe_static_cast<int,size_t>(jobs_.size()); state.lastJobIndex++)
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
					state.lastJobIndex < safe_static_cast<int,size_t>(jobs_.size()); state.lastJobIndex++) 
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

						// done
						doneGuard_ = 0;
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
		if (state.nextJobIndex >= safe_static_cast<int,size_t>(jobs_.size())) {
			return resFalgs | RunFlag::DONE;
		}

		// execute the next job
		{
			JobData& job = jobs_[state.nextJobIndex];

			core::StopWatch timer;

			job.pJobRun(job.pData, 
				job.batchOffset, 
				job.batchNum,
				threadIdx);

			job.done = true;

			const TimeVal elapsed = timer.GetTimeVal();

			job.execTime = elapsed;
			stats_.threadExecTime[threadIdx] += elapsed;

			if (jobListRunner::var_LongJobMs > 0)
			{
				// we allow jobs with no priority to run as long as they want
				if (elapsed.GetMilliSeconds() > jobListRunner::var_LongJobMs 
					&& getPriority() != JobListPriority::NONE)
				{				
					X_WARNING("jobListRunner", "a single job took more than: %ims elapsed: %gms "
						"pFunc: %p pData: %p batchOffset: %" PRIu32 " batchNum: %" PRIu32,
						jobListRunner::var_LongJobMs,
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


void JobList::PreSubmit(JobList* pWaitFor)
{
	currentJob_ = 0;
	syncCount_ = safe_static_cast<int32_t, size_t>(jobs_.size());

	if(pWaitFor) {
			waitForList_ = &pWaitFor->doneGuard_;
	}

	doneGuard_ = 1;

	JobData endJob;
	endJob.pJobRun = NopJob;
	endJob.pData = &JOB_LIST_DONE;
	endJob.done = false;
	endJob.batchOffset = 0;
	endJob.batchNum = 0;
	jobs_.append(endJob);

	stats_.submitTime = GetTimeReal();

	isSubmit_ = true;
	isDone_ = false;
}


TimeVal JobList::GetTimeReal(void) const
{
	return pTimer_->GetTimeNowNoScale();
}
// ----------------------------------

JobThread::threadJobList::threadJobList()
{
	jobList = nullptr;
	version = 0;
}

JobThread::JobThread() :
	signalWorkerDone_(false),
	signalMoreWorkToDo_(false)
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

X_DISABLE_WARNING(4127)
	while (true)
X_ENABLE_WARNING(4127)
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

				core::StopWatch timer;

				signalMoreWorkToDo_.wait();

				stats_.waitforJobTime += timer.GetTimeVal();
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
			for (size_t i = 0; i < jobStates.size(); i++) 
			{
				const JobList* pList = jobStates[i].jobList;
				if (pList->getPriority() > priority && pList->CanRun()) 
				{
					priority = pList->getPriority();
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

			for (size_t i = 0; i < jobStates.size(); i++) 
			{
				const JobList* pList = jobStates[i].jobList;
				if (i != lastStalledJobList && pList->getPriority() > priority && pList->CanRun()) 
				{
					priority = pList->getPriority();
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

TimeVal JobThread::GetTimeReal(void) const
{
	return gEnv->pTimer->GetTimeNowNoScale();
}


// ----------------------------------

jobListRunner::jobListRunner() :
numThreads_(0)
{

}


jobListRunner::~jobListRunner()
{

}

void jobListRunner::StartUp(void)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pCore);
	X_ASSERT_NOT_NULL(gEnv->pConsole);

	// get the num HW threads
	ICore* pCore = (ICore*)gEnv->pCore;
	const CpuInfo* pCpu = pCore->GetCPUInfo();

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

void jobListRunner::StartThreads(void)
{
	X_LOG0("jobListRunner", "Creating %" PRIi32 " threads", numThreads_);

	int32_t i;
	for (i = 0; i < numThreads_; i++)
	{
		core::StackString<64> name;
		name.appendFmt("Worker_%" PRIi32, i);
		threads_[i].setThreadIdx(i);
		threads_[i].Create(name.c_str()); // default stack size.
		threads_[i].Start();
	}

}

void jobListRunner::ShutDown(void)
{
	X_LOG0("jobListRunner", "Shutting Down");

	int32_t i;

	for (i = 0; i < numThreads_; i++) {
		threads_[i].Stop(); 
	}
	for (i = 0; i < numThreads_; i++) {
		threads_[i].Join();
	}
}


void jobListRunner::SubmitJobList(JobList* pList, JobList* pWaitFor)
{
	X_ASSERT_NOT_NULL(pList);

	core::CriticalSection::ScopedLock lock(addJobListCrit_);

	pList->PreSubmit(pWaitFor);

	for (int32_t i = 0; i < numThreads_; i++) {
		threads_[i].AddJobList(pList);
		threads_[i].SignalWork();
	}
}

void jobListRunner::WaitForThreads(void)
{
	for (int32_t i = 0; i < numThreads_; i++) {
		threads_[i].WaitForThread();
	}
}

int32_t jobListRunner::numThreads(void) const
{
	return numThreads_;
}

} // namespace JobList

X_NAMESPACE_END