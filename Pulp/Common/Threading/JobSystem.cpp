#include <EngineCommon.h>
#include "JobSystem.h"

#include "Util\Cpu.h"
#include "Time\StopWatch.h"

#include "ICore.h"
#include "ITimer.h"
#include "IConsole.h"

X_NAMESPACE_BEGIN(core)

int JobSystem::var_LongJobMs = 8;

// -------------------

JobThread::JobThread() :
    signalWorkerDone_(false),
    signalMoreWorkToDo_(false)
{
    core::zero_object(ques_);

    moreWorkToDo_ = false;
    threadIdx_ = 0;

    pTimer_ = nullptr;
}

JobThread::~JobThread()
{
}

void JobThread::init(uint32_t idx, ThreadQueue* pQues)
{
    threadIdx_ = idx;

    for (size_t i = 0; i < JobPriority::ENUM_COUNT; i++) {
        ques_[i] = &pQues[i];
    }

    X_ASSERT_NOT_NULL(gEnv->pTimer);
    pTimer_ = gEnv->pTimer;
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
    ThreadAbstract::stop();
    SignalWork();
}

void JobThread::WaitForThread(void)
{
    signalWorkerDone_.wait();
}

TimeVal JobThread::GetTimeReal(void) const
{
    return pTimer_->GetTimeNowNoScale();
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

                if (moreWorkToDo_) {
                    moreWorkToDo_ = false;
                    signalMoreWorkToDo_.clear();
                    signalCritical_.Leave();
                }
                else {
                    signalWorkerDone_.raise();
                    signalCritical_.Leave();

                    core::StopWatch timer;

                    signalMoreWorkToDo_.wait();

                    stats_.waitforJobTime += timer.GetTimeVal();
                    continue;
                }
            }

            if (!thread.shouldRun()) {
                break;
            }

            retVal = ThreadRunInternal(thread);
        }

    signalWorkerDone_.raise();

    return retVal;
}

Thread::ReturnValue JobThread::ThreadRunInternal(const Thread& thread)
{
    JobDecl job;
    size_t i;

    while (thread.shouldRun()) {
        // pop from the ques in order.
        for (i = 0; i < JobPriority::ENUM_COUNT; i++) {
            if (ques_[i]->tryPop(job)) {
                break;
            }
        }

        // got a job?
        if (i == JobPriority::ENUM_COUNT) {
            return Thread::ReturnValue(0);
        }

        {
            core::StopWatch timer;

            // run it.
            job.pJobFunc(job.pParam, threadIdx_);

            job.execTime = timer.GetTimeVal();

            if (JobSystem::var_LongJobMs > 0) {
                // we allow jobs with no priority to run as long as they want
                if (job.execTime.GetMilliSeconds() > JobSystem::var_LongJobMs
                    && i != JobPriority::NONE) {
                    X_WARNING("JobSystem", "a single job took more than: %ims elapsed: %gms "
                                           "pFunc: %p pData: %p",
                        JobSystem::var_LongJobMs,
                        job.execTime.GetMilliSeconds(),
                        job.pJobFunc,
                        job.pParam);
                }
            }
        }
    }
    return Thread::ReturnValue(0);
}

// ======================================

JobSystem::JobSystem(core::MemoryArenaBase* arena) :
    numThreads_(0),
    ques_{arena, arena, arena}
{
}

JobSystem::~JobSystem()
{
}

bool JobSystem::StartUp(void)
{
    X_ASSERT_NOT_NULL(gEnv);
    X_ASSERT_NOT_NULL(gEnv->pArena);
    X_ASSERT_NOT_NULL(gEnv->pCore);
    X_ASSERT_NOT_NULL(gEnv->pConsole);

    // get the num HW threads
    ICore* pCore = gEnv->pCore;
    const CpuInfo* pCpu = pCore->GetCPUInfo();

    int32_t numCores = pCpu->GetCoreCount();
    numThreads_ = core::Max(core::Min(HW_THREAD_MAX, numCores - HW_THREAD_NUM_DELTA), 1u);

    // create a var.
    ADD_CVAR_REF("jobsys_numThreads", numThreads_, numThreads_, 1,
        HW_THREAD_MAX, core::VarFlag::SYSTEM,
        "The number of threads used by the job system");

    // register longJob
    ADD_CVAR_REF("jobsys_longJobMs", var_LongJobMs, var_LongJobMs, 0, 32, core::VarFlag::SYSTEM,
        "If a single job takes longer than this a warning is printed. 0 = disabled");

    return StartThreads();
}

void JobSystem::AddJob(const JobDecl job, JobPriority::Enum priority)
{
    ques_[priority].push(job);

    // signal
    for (int32_t i = 0; i < numThreads_; i++) {
        threads_[i].SignalWork();
    }
}

void JobSystem::AddJobs(JobDecl* pJobs, size_t numJobs, JobPriority::Enum priority)
{
    X_ASSERT_NOT_NULL(pJobs);

    for (size_t i = 0; i < numJobs; i++) {
        ques_[priority].push(pJobs[i]);
    }

    // signal
    for (int32_t i = 0; i < numThreads_; i++) {
        threads_[i].SignalWork();
    }
}

void JobSystem::waitForAllJobs(void)
{
    int32_t i;
    for (i = 0; i < JobPriority::ENUM_COUNT; i++) {
        while (ques_[i].isNotEmpty()) {
            core::Thread::yield();
        }
    }

    for (i = 0; i < numThreads_; i++) {
        threads_[i].WaitForThread();
    }
}

int32_t JobSystem::numThreads(void) const
{
    return numThreads_;
}

bool JobSystem::StartThreads(void)
{
    X_LOG0("JobSystem", "Creating %i threads", numThreads_);

    int32_t i;
    for (i = 0; i < numThreads_; i++) {
        core::StackString<64> name;
        name.appendFmt("JobSystem::Worker_%i", i);
        threads_[i].init(i, ques_);
        threads_[i].create(name.c_str()); // default stack size.
        threads_[i].start();
    }

    return true;
}

void JobSystem::ShutDown(void)
{
    X_LOG0("JobSystem", "Shutting Down");

    int32_t i;

    for (i = 0; i < numThreads_; i++) {
        threads_[i].Stop();
    }
    for (i = 0; i < numThreads_; i++) {
        threads_[i].join();
    }
}

X_NAMESPACE_END