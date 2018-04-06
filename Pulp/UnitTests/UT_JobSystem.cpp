#include "stdafx.h"

#include "Threading\JobSystem.h"
#include "Time\StopWatch.h"
#include "Time\TimeVal.h"

X_USING_NAMESPACE;

using namespace core;

namespace
{
    core::AtomicInt numJobsRan(0);

    X_PRAGMA(optimize("", off))
    void TestJob(void* pParam, uint32_t workerIdx)
    {
        size_t idx = reinterpret_cast<size_t>(pParam);

        size_t i, running_total = 23;
        size_t some_large_number = 0x100000;
        for (i = 0; i < some_large_number; i++) {
            running_total = 37 * running_total + i;
        }

        // X_LOG0("TestJob", "job idx: %i threadIdx: %i", idx, workerIdx);

        ++numJobsRan;
    }
    X_PRAGMA(optimize("", on))

} // namespace

TEST(Threading, JobSystem)
{
    JobSystem jobSys(g_arena);

    jobSys.StartUp();

    const size_t numJobs = 0x100;

    core::StopWatch timer;
    core::TimeVal singleThreadElapse;
    {
        timer.Start();

        size_t i;
        for (i = 0; i < numJobs; i++) {
            TestJob((void*)(i), 0);
        }

        singleThreadElapse = timer.GetTimeVal();

        X_LOG0("jobListRunner", "Single threaded exec time: %f", singleThreadElapse.GetMilliSeconds());
    }

    numJobsRan = 0;

    core::TimeVal MultiElapsed;
    {
        timer.Start();

        // create jobs on stack.
        JobDecl jobs[numJobs];
        size_t i;

        for (i = 0; i < numJobs; i++) {
            jobs[i] = JobDecl(TestJob, (void*)(i));
        }

        jobSys.AddJobs(jobs, numJobs, JobPriority::HIGH);
        jobSys.waitForAllJobs();

        MultiElapsed = timer.GetTimeVal();

        EXPECT_EQ(numJobs, numJobsRan);
    }

    float32_t percentage = static_cast<float32_t>(singleThreadElapse.GetValue()) / static_cast<float32_t>(MultiElapsed.GetValue());

    percentage *= 100;

    // print the stats.
    X_LOG0("JobSystem", "Stats");
    X_LOG_BULLET;
    X_LOG0("JobSystem", "SingleThreaded: %g", singleThreadElapse.GetMilliSeconds());
    X_LOG0("JobSystem", "MultiThreaded: %g", MultiElapsed.GetMilliSeconds());
    X_LOG0("JobSystem", "Percentage: %g%% scaling: %g%%", percentage, percentage / jobSys.numThreads());

    jobSys.ShutDown();
}

namespace
{
    static void EmptyJob(void* pParam, uint32_t workerIdx)
    {
    }
} // namespace

TEST(Threading, JobSystemEmpty)
{
    JobSystem jobSys(g_arena);
    jobSys.StartUp();

    const size_t numJobs = 65000;

    core::TimeVal MultiElapsed;
    core::StopWatch timer;
    {
        timer.Start();

        size_t i;

        for (i = 0; i < numJobs; i++) {
            JobDecl job(EmptyJob, 0);

            jobSys.AddJob(job, JobPriority::HIGH);
        }

        jobSys.waitForAllJobs();

        MultiElapsed = timer.GetTimeVal();
    }

    X_LOG0("JobSystem", "%" PRIuS " empty jobs: %gms", numJobs, MultiElapsed.GetMilliSeconds());

    jobSys.ShutDown();
}