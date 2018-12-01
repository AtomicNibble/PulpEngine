#include "EngineCommon.h"
#include "JobSystem2.h"

#include "Random\MultiplyWithCarry.h"
#include "Random\XorShift.h"

#include "Util\Cpu.h"
#include <Memory\VirtualMem.h>

#include <Time\StopWatch.h>

#include <atomic>

X_NAMESPACE_BEGIN(core)

namespace V2
{
    // ===================================

#if X_ENABLE_JOBSYS_PROFILER

    void JobSystemStats::clear(void)
    {
        jobsStolen = 0;
        jobsRun = 0;
        jobsAssited = 0;
        workerUsedMask = 0;
        workerAwokenMask = 0;
    }

    // ===================================

    JobQueueHistory::FrameHistory::FrameHistory()
    {
        bottom_ = 0;
        top_ = 0;
    }

    JobQueueHistory::JobQueueHistory()
    {
        currentIdx_ = 0;
    }

    JobQueueHistory::~JobQueueHistory()
    {
    }

    void JobQueueHistory::sethistoryIndex(int32_t historyIdx)
    {
        int32_t newIdx = (historyIdx) & (JOBSYS_HISTORY_COUNT - 1);
        auto start = core::StopWatch::GetTimeNow();

        auto& history = frameHistory_[newIdx];
        history.start = start;
        atomic::Exchange<long>(&history.bottom_, 0);
        atomic::Exchange<long>(&history.top_, 0);

        COMPILER_BARRIER_W;

        currentIdx_ = newIdx;
    }

    X_INLINE JobQueueHistory::FrameHistory& JobQueueHistory::getCurFrameHistory(void)
    {
        return frameHistory_[currentIdx_];
    }

#endif // !X_ENABLE_JOBSYS_PROFILER

    // ===================================

    ThreadQue::ThreadQue()
    {
        bottom_ = 0;
        top_ = 0;
    }

    void ThreadQue::Push(Job* job)
    {
        long b = bottom_;
        jobs_[b & MASK] = job;

        // ensure the job is written before b+1 is published to other threads.
        COMPILER_BARRIER_W;

        bottom_ = b + 1;
    }

    Job* ThreadQue::Pop(void)
    {
        long b = bottom_ - 1;

        atomic::Exchange(&bottom_, b);

        long t = top_;
        if (t <= b) {
            // non-empty queue
            Job* job = jobs_[b & MASK];
            if (t != b) {
                // there's still more than one item left in the queue
                return job;
            }

            // this is the last item in the queue
            if (atomic::CompareExchange(&top_, t + 1, t) != t) {
                // failed race against steal operation
                job = nullptr;
            }

            bottom_ = t + 1;
            return job;
        }
        else {
            // deque was already empty
            bottom_ = t;
            return nullptr;
        }
    }

    Job* ThreadQue::Steal(void)
    {
        long t = top_;

        // ensure that top is always read before bottom.
        // loads will not be reordered with other loads on x86, so a compiler barrier is enough.
        COMPILER_BARRIER_R;

        long b = bottom_;
        if (t < b) {
            // non-empty queue
            Job* job = jobs_[t & MASK];

            // the interlocked function serves as a compiler barrier, and guarantees that the read happens before the CAS.
            if (atomic::CompareExchange(&top_, t + 1, t) != t) {
                // a concurrent steal or pop operation removed an element from the deque in the meantime.
                return nullptr;
            }
            return job;
        }
        else {
            // empty queue
            return nullptr;
        }
    }

    X_INLINE size_t ThreadQue::rand(void)
    {
        return rand_.rand();
    }

    // ===================================

    JobSystem::ThreadJobAllocator::ThreadJobAllocator()
    {
        allocated = 0;
    }

    JobSystem::JobSystem(core::MemoryArenaBase* arena) :
        arena_(arena)
    {
        numThreads_ = 0;
        numQueue_ = 0;
        core::zero_object(pThreadQues_);
        core::zero_object(pJobAllocators_);
#if X_ENABLE_JOBSYS_PROFILER
        core::zero_object(pTimeLines_);
#endif // !X_ENABLE_JOBSYS_PROFILER
    }

    JobSystem::~JobSystem()
    {
    }

    bool JobSystem::StartUp(uint32_t threadCount)
    {
        X_ASSERT_NOT_NULL(gEnv);
        X_ASSERT_NOT_NULL(gEnv->pCore);
        X_ASSERT_NOT_NULL(gEnv->pConsole);
        X_PROFILE_NO_HISTORY_BEGIN("JobSysInit", core::profiler::SubSys::CORE);

        if (threadCount == AUTO_THREAD_COUNT) {
            ICore* pCore = gEnv->pCore;
            const CpuInfo* pCpu = pCore->GetCPUInfo();

            uint32_t numCores = pCpu->GetCoreCount();
            numThreads_ = math<uint32_t>::clamp(numCores - HW_THREAD_NUM_DELTA, 1u, HW_THREAD_MAX);
        }
        else {
            numThreads_ = threadCount;
        }

        // main and IO job list space.
        numThreads_ = math<uint32_t>::clamp(numThreads_, 1u, HW_THREAD_MAX - 2);

        // HACK: we have some jobs that wait on other jobs.
        // so we need atleast two.
        numThreads_ = math<uint32_t>::max(numThreads_, 2u);

        if (!StartThreads()) {
            X_ERROR("Scheduler", "Failed to start worker threads");
            return false;
        }

        return true;
    }

    void JobSystem::ShutDown(void)
    {
        size_t i;
        for (i = 0; i < numThreads_; i++) {
            threads_[i].stop();
        }

        cond_.NotifyAll();

        for (i = 0; i < numThreads_; i++) {
            threads_[i].join();
        }

        // clean up allocators and ques.
        for (i = 0; i < HW_THREAD_MAX; i++) {
            if (pThreadQues_[i]) {
                X_DELETE(pThreadQues_[i], arena_);
            }
            if (pJobAllocators_[i]) {
                X_DELETE(pJobAllocators_[i], arena_);
            }
#if X_ENABLE_JOBSYS_PROFILER
            if (pTimeLines_[i]) {
                X_DELETE(pTimeLines_[i], arena_);
            }
#endif // !X_ENABLE_JOBSYS_PROFILER
        }

        threadIdToIndex_.clear();

        core::zero_object(pThreadQues_);
        core::zero_object(pJobAllocators_);
#if X_ENABLE_JOBSYS_PROFILER
        core::zero_object(pTimeLines_);
#endif // !X_ENABLE_JOBSYS_PROFILER

        // clear main thread tls values also.
        ThreadQue_.setValue(nullptr);
        ThreadAllocator_.setValue(nullptr);
    }

    void JobSystem::OnFrameBegin(bool isProfilerPaused)
    {
#if !X_ENABLE_JOBSYS_PROFILER
        X_UNUSED(isProfilerPaused);
#else

        if (!isProfilerPaused) {
            currentHistoryIdx_ = (currentHistoryIdx_ + 1) & (JOBSYS_HISTORY_COUNT - 1);
        }

        stats_[currentHistoryIdx_].clear();

        for (uint32_t i = 0; i < HW_THREAD_MAX; i++) {
            if (pTimeLines_[i]) {
                pTimeLines_[i]->sethistoryIndex(currentHistoryIdx_);
            }
        }

#endif // !X_ENABLE_JOBSYS_PROFILER
    }

    /// ===============================================

    void JobSystem::CreateQueForCurrentThread(void)
    {
        X_ASSERT(numThreads_ > 0, "Can't create que for thread before StartUp has been called")(numThreads_);

        uint32_t threadId = Thread::getCurrentID();

        if (CurrentThreadHasWorkerQueue()) {
            X_WARNING("JobSys", "Thread 0x%x already has a que", threadId);
            return;
        }

        CreateThreadObjects(threadId);
        {
            size_t threadIdx = GetThreadIndex();
            ThreadQue_.setValue(pThreadQues_[threadIdx]);
            ThreadAllocator_.setValue(pJobAllocators_[threadIdx]);
        }
    }

    uint32_t JobSystem::GetThreadCount(void) const
    {
        return numThreads_;
    }

    uint32_t JobSystem::GetQueueCount(void) const
    {
        return numQueue_;
    }

    JobSystem::ThreadIdArray JobSystem::getThreadIds(void)
    {
        ThreadIdArray arr;

        for (uint32_t i = 0; i < numThreads_; i++) {
            arr.push_back(threads_[i].getID());
        }

        return arr;
    }

    /// ===============================================

    bool JobSystem::StartThreads(void)
    {
        X_LOG0("JobSystemV2", "Creating %" PRIu32 " threads", numThreads_);

        // que for main thread also.
        uint32_t threadId = Thread::getCurrentID();

        CreateThreadObjects(threadId);
        {
            const size_t threadIdx = GetThreadIndex();
            ThreadQue_.setValue(pThreadQues_[threadIdx]);
            ThreadAllocator_.setValue(pJobAllocators_[threadIdx]);
        }

        uint32_t i;
        for (i = 0; i < numThreads_; i++) {
            core::StackString<64> name;
            name.appendFmt("JobSystemV2::Worker_%" PRIu32, i);
            threads_[i].setData(this);
            threads_[i].create(name.c_str()); // default stack size.

            threadId = threads_[i].getID();
            CreateThreadObjects(threadId);
        }
        for (i = 0; i < numThreads_; i++) {
            threads_[i].start(ThreadRun_s);
        }

        return true;
    }

    void JobSystem::CreateThreadObjects(uint32_t threadId)
    {
        const uint32_t idx = safe_static_cast<uint32_t, size_t>(threadIdToIndex_.size());

        X_ASSERT(idx < HW_THREAD_MAX, "No room for thread que")(idx, HW_THREAD_MAX);
        X_ASSERT(pThreadQues_[idx] == nullptr, "Double init")(threadId, idx);

        pThreadQues_[idx] = X_NEW(ThreadQue, arena_, "JobThreadQue");
        pJobAllocators_[idx] = X_NEW(ThreadJobAllocator, arena_, "JobThreadAllocator");
        threadIdToIndex_.push_back(std::make_pair(threadId, idx));

        X_ASSERT_ALIGNMENT(pThreadQues_[idx], 64, 0);
        X_ASSERT_ALIGNMENT(&pJobAllocators_[idx]->jobs, 64, 0);

#if X_ENABLE_JOBSYS_PROFILER
        pTimeLines_[idx] = X_NEW(JobQueueHistory, arena_, "JobThreadTimeline");
#endif // !X_ENABLE_JOBSYS_PROFILER

        numQueue_ = idx + 1;
    }

    /// ===============================================

    Job* JobSystem::CreateJob(JobFunction::Pointer function JOB_SYS_SUB_PARAM)
    {
        Job* job = AllocateJob();
        job->pFunction = function;
        job->pParent = nullptr;
        // does this need to be atomic?
        // i think so since second time it's used the value might be stale.
        atomic::Exchange(&job->unfinishedJobs, 1);
        atomic::Exchange(&job->continuationCount, 0);
        job->runFlags = 0;

#if X_ENABLE_JOBSYS_RECORD_SUBSYSTEM
        job->subSystem = subSystem;
#endif // !X_ENABLE_JOBSYS_RECORD_SUBSYSTEM

#if X_DEBUG
        core::zero_object(job->continuations);
#endif // X_DEBUG

        return job;
    }

    Job* JobSystem::CreateJobAsChild(Job* pParent, JobFunction::Pointer function JOB_SYS_SUB_PARAM)
    {
        atomic::Increment(&pParent->unfinishedJobs);

        Job* job = AllocateJob();
        job->pFunction = function;
        job->pParent = pParent;
        atomic::Exchange(&job->unfinishedJobs, 1);
        atomic::Exchange(&job->continuationCount, 0);
        job->runFlags = 0;

#if X_ENABLE_JOBSYS_RECORD_SUBSYSTEM
        job->subSystem = subSystem;
#endif // !X_ENABLE_JOBSYS_RECORD_SUBSYSTEM

#if X_DEBUG
        core::zero_object(job->continuations);
#endif // X_DEBUG

        return job;
    }

    /// ===============================================

    void JobSystem::Run(Job* pJob)
    {
        ThreadQue* queue = GetWorkerThreadQueue();
        queue->Push(pJob);

        cond_.NotifyOne();
    }

    void JobSystem::Wait(Job* pJob)
    {
        size_t threadIdx = GetThreadIndex();
        ThreadQue& threadQue = *GetWorkerThreadQueue(threadIdx);

        while (!HasJobCompleted(pJob)) {
            Job* nextJob = GetJob(threadQue);
            if (nextJob) {
#if X_ENABLE_JOBSYS_PROFILER
                ++stats_[currentHistoryIdx_].jobsAssited;
#endif // !X_ENABLE_JOBSYS_PROFILER

                Execute(nextJob, threadIdx);
            }
        }
    }

    void JobSystem::WaitWithoutHelp(Job* pJob) const
    {
        while (!HasJobCompleted(pJob)) {
            Thread::yield();
        }
    }

    bool JobSystem::HelpWithWork(void)
    {
        X_ASSERT(CurrentThreadHasWorkerQueue(), "HelpWithWork called on thread that has no que")(core::Thread::getCurrentID());

        size_t threadIdx = GetThreadIndex();
        ThreadQue& threadQue = *GetWorkerThreadQueue(threadIdx);

        Job* nextJob = GetJob(threadQue);
        if (nextJob) {
#if X_ENABLE_JOBSYS_PROFILER
            ++stats_[currentHistoryIdx_].jobsAssited;
#endif // !X_ENABLE_JOBSYS_PROFILER

            Execute(nextJob, threadIdx);
            return true;
        }

        return false;
    }

    /// ===============================================

    Job* JobSystem::AllocateJob(void)
    {
        ThreadJobAllocator* pAllocator = GetWorkerThreadAllocator();

        const uint32_t index = pAllocator->allocated++;

#if X_ENABLE_JOBSYS_PROFILER
        size_t threadIdx = GetThreadIndex();

        Job* pJob = &pAllocator->jobs[index & (JobSystem::MAX_JOBS - 1u)];
        pJob->origThreadIdx = safe_static_cast<uint8_t>(threadIdx);
        return pJob;
#else
        return &pAllocator->jobs[index & (JobSystem::MAX_JOBS - 1u)];
#endif // !X_ENABLE_JOBSYS_PROFILER
    }

    Job* JobSystem::AllocateJob(size_t threadIdx)
    {
        ThreadJobAllocator* pAllocator = GetWorkerThreadAllocator(threadIdx);

        const uint32_t index = pAllocator->allocated++;

#if X_ENABLE_JOBSYS_PROFILER
        Job* pJob = &pAllocator->jobs[index & (JobSystem::MAX_JOBS - 1u)];
        pJob->origThreadIdx = safe_static_cast<uint8_t>(threadIdx);
        return pJob;
#else
        return &pAllocator->jobs[index & (JobSystem::MAX_JOBS - 1u)];
#endif // !X_ENABLE_JOBSYS_PROFILER
    }

    void JobSystem::FreeJob(Job* pJob)
    {
        X_UNUSED(pJob);
        X_ASSERT_NOT_IMPLEMENTED();
    }

    /// ===============================================

    bool JobSystem::CurrentThreadHasWorkerQueue(void) const
    {
        if (!CurrentThreadHasIndex()) {
            return false;
        }

        return GetWorkerThreadQueue() != nullptr;
    }

    ThreadQue* JobSystem::GetWorkerThreadQueue(void) const
    {
        return ThreadQue_.getValue<ThreadQue>();
    }

    ThreadQue* JobSystem::GetWorkerThreadQueue(size_t threadIdx) const
    {
        return pThreadQues_[threadIdx];
    }

    JobSystem::ThreadJobAllocator* JobSystem::GetWorkerThreadAllocator(void) const
    {
        return ThreadAllocator_.getValue<ThreadJobAllocator>();
    }

    JobSystem::ThreadJobAllocator* JobSystem::GetWorkerThreadAllocator(size_t threadIdx) const
    {
        return pJobAllocators_[threadIdx];
    }

    /// ===============================================

    Job* JobSystem::GetJob(void)
    {
        ThreadQue* queue = GetWorkerThreadQueue();
        return GetJob(*queue);
    }

    Job* JobSystem::GetJob(ThreadQue& queue)
    {
        Job* pJob = queue.Pop();
        if (!IsEmptyJob(pJob)) {
            return pJob;
        }

        // this is not a valid job because our own queue is empty, so try stealing from some other queue
        size_t randomIndex = queue.rand() % numQueue_;

        ThreadQue* stealQueue = pThreadQues_[randomIndex];
        if (stealQueue == &queue) {
            // don't try to steal from ourselves
            // but if we not main que try steal from that
            return nullptr;
        }

        Job* stolenJob = stealQueue->Steal();
        if (IsEmptyJob(stolenJob)) {
            // we couldn't steal a job from the other queue either, so we just yield our time slice for now
            return nullptr;
        }

#if X_ENABLE_JOBSYS_PROFILER
        ++stats_[currentHistoryIdx_].jobsStolen;
#endif // !X_ENABLE_JOBSYS_PROFILER

        return stolenJob;
    }

    Job* JobSystem::GetJobCheckAllQues(ThreadQue& queue)
    {
        Job* pJob = queue.Pop();
        if (!IsEmptyJob(pJob)) {
            return pJob;
        }
        for (uint32_t i = 0; i < numQueue_; i++) {
            ThreadQue* stealQueue = pThreadQues_[i];
            if (stealQueue == &queue) {
                continue;
            }

            Job* stolenJob = stealQueue->Steal();
            if (!IsEmptyJob(stolenJob)) {
#if X_ENABLE_JOBSYS_PROFILER
                ++stats_[currentHistoryIdx_].jobsStolen;
#endif // !X_ENABLE_JOBSYS_PROFILER

                return stolenJob;
            }
        }

        return nullptr;
    }

    void JobSystem::Execute(Job* pJob, size_t threadIdx)
    {
#if X_ENABLE_JOBSYS_PROFILER
        auto* pTimeLine = pTimeLines_[threadIdx];
        const int32_t currentHistoryIdx = currentHistoryIdx_;
        auto& history = pTimeLine->getCurFrameHistory();
        auto* pEntry = &history.entryes_[history.bottom_ & JobQueueHistory::MASK];

        // we must write this before running the job.
        // as the job may run another job that then runs on this thread.
        // which will mean it will get incorrect index.
        // ..
        // This reuslts in another problem tho, history entry might not of been written fully
        // before it's read in the profiler vis.
        // to solve this, the profiler must use getMaxreadIdx(), which returns (bottom_ - 1)
        // and at the end of the frame we bump bottom + 1 allowing the profiler to read
        // the last entry.
        ++history.bottom_;

        core::StopWatch timer;
#endif // !X_ENABLE_JOBSYS_PROFILER

        (pJob->pFunction)(*this, threadIdx, pJob, pJob->pArgData);

#if X_ENABLE_JOBSYS_PROFILER
        pEntry->start = timer.GetStart();
        pEntry->end = timer.GetEnd();

        // work out the id.
        ThreadJobAllocator* pThreadAlloc = GetWorkerThreadAllocator(pJob->origThreadIdx);
        ptrdiff_t jobIdx = (pJob - pThreadAlloc->jobs);

        pEntry->id.threadIdx = pJob->origThreadIdx;
        pEntry->id.jobIdx = safe_static_cast<uint16_t>(jobIdx);
#if X_ENABLE_JOBSYS_RECORD_SUBSYSTEM
        pEntry->subsystem = pJob->subSystem;
#else
        pEntry->subsystem = core::profiler::SubSys::UNCLASSIFIED;
#endif // !X_ENABLE_JOBSYS_RECORD_SUBSYSTEM

        COMPILER_BARRIER_W;

        // mark it readable.
        ++history.top_;

        ++stats_[currentHistoryIdx].jobsRun;
        stats_[currentHistoryIdx].workerUsedMask |= static_cast<int32_t>(BIT(threadIdx));

#endif // !X_ENABLE_JOBSYS_PROFILER

        Finish(pJob, threadIdx);
    }

    void JobSystem::Finish(Job* pJob, size_t threadIdx)
    {
        const int32_t unfinishedJobs = atomic::Decrement(&pJob->unfinishedJobs);
        if (unfinishedJobs == 0) {
            // now i only run continuations after all child jobs are complete.
            const int32_t continuationCount = pJob->continuationCount;
            if (continuationCount != 0) {
                ThreadQue* queue = GetWorkerThreadQueue(threadIdx);

                const int32_t flags = pJob->runFlags;
                for (int32_t i = 0; i < continuationCount; i++) {
                    const Job::JobId& id = pJob->continuations[i];
                    ThreadJobAllocator* pThreadAlloc = GetWorkerThreadAllocator(id.threadIdx);

                    Job* pContinuation = &pThreadAlloc->jobs[id.jobIdx];

                    // run inline?
                    if (flags != 0 && core::bitUtil::IsBitSet(flags, i)) {
                        Execute(pContinuation, threadIdx);
                    }
                    else {
                        queue->Push(pContinuation);
                    }
                }

                // wake up other threads they may be all asleep.
                cond_.NotifyOne();
            }

            // if we are child of a job, dec parents counter.
            // when all child jobs are done the parent becomes complete.
            if (pJob->pParent) {
                Finish(pJob->pParent, threadIdx);
            }
        }
    }

    size_t JobSystem::GetThreadIndex(void) const
    {
        const uint32_t threadId = Thread::getCurrentID();

        for (auto id : threadIdToIndex_) {
            if (id.first == threadId) {
                return id.second;
            }
        }

        X_ASSERT_UNREACHABLE();
        return 0;
    }

    bool JobSystem::CurrentThreadHasIndex(void) const
    {
        const uint32_t threadId = Thread::getCurrentID();

        for (auto id : threadIdToIndex_) {
            if (id.first == threadId) {
                return true;
            }
        }

        return false;
    }

    Thread::ReturnValue JobSystem::ThreadRun_s(const Thread& thread)
    {
        JobSystem* pJobSystem = static_cast<JobSystem*>(thread.getData());
        return pJobSystem->ThreadRun(thread);
    }

    void JobSystem::ThreadBackOff(int32_t backoff)
    {
        if (backoff < 10 && backoff > 0) {
            Thread::yieldProcessor();
        }
        else if (backoff < 20) {
            for (size_t i = 0; i != 50; i += 1) {
                Thread::yieldProcessor();
            }
        }
        else if (backoff < 28) {
            Thread::yield();
        }

        Thread::sleep(0);
    }

    Thread::ReturnValue JobSystem::ThreadRun(const Thread& thread)
    {
        const size_t threadIdx = GetThreadIndex();
        ThreadQue& threadQue = *GetWorkerThreadQueue(threadIdx);
        ThreadJobAllocator* pThreadAlloc = GetWorkerThreadAllocator(threadIdx);

        ThreadQue_.setValue(&threadQue);
        ThreadAllocator_.setValue(pThreadAlloc);

        int32_t backoff = 0;

        {
            CriticalSection::ScopedLock lock(condCS_);
            if (thread.shouldRun()) // check after we got lock, as may have shutdown right away.
            {
                cond_.Wait(condCS_);
            }
#if X_ENABLE_JOBSYS_PROFILER
            stats_[currentHistoryIdx_].workerAwokenMask |= static_cast<int32_t>(BIT(threadIdx));
#endif // !X_ENABLE_JOBSYS_PROFILER
        }

        while (thread.shouldRun()) {
            Job* pJob = GetJob(threadQue);
            if (pJob) {
                backoff = 0;
                Execute(pJob, threadIdx);
            }
            else {
                ThreadBackOff(backoff);
                backoff++;

                if (backoff > 25) {
                    // if we not had a job for a while, ensure all que's are empty
                    // if so wait on the consition.
                    pJob = GetJobCheckAllQues(threadQue);
                    if (pJob) {
                        backoff = 0;
                        Execute(pJob, threadIdx);
                    }
                    else {
                        CriticalSection::ScopedLock lock(condCS_);
                        cond_.Wait(condCS_);
                    }
                }
            }
        }

        // null the tls (not needed)
        ThreadQue_.setValue(nullptr);
        ThreadAllocator_.setValue(nullptr);

        return Thread::ReturnValue(0);
    }

} // namespace V2

X_NAMESPACE_END
