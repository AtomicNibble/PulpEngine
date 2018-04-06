#include "stdafx.h"

#include "Threading\JobSystem2.h"
#include "Time\StopWatch.h"
#include "Time\TimeVal.h"

X_USING_NAMESPACE;

using namespace core;
using namespace core::V2;

namespace
{
    core::AtomicInt numJobsRan(0);

    static void EmptyJob(JobSystem& jobSys, size_t threadIdx, Job* job, void* pParam)
    {
    }

    static void UpdateCamel(uint32_t*, size_t count)
    {
        ++numJobsRan;
    }

    struct Goat
    {
        int32_t goat;
        int32_t boat;
    };

    void UpdateGoat(Goat*, size_t count)
    {
    }

} // namespace

TEST(Threading, JobSystem2Empty)
{
    JobSystem jobSys(g_arena);
    jobSys.StartUp();

    core::StopWatch timer;

    const size_t numJobs = 4000;
    const size_t numRuns = 10;

    std::array<float, numRuns> times;

    for (size_t n = 0; n < numRuns; n++) {
        core::TimeVal addTime;
        core::TimeVal runTime;
        {
            timer.Start();

            size_t i;

            Job* root = jobSys.CreateJob(&EmptyJob JOB_SYS_SUB_ARG(core::profiler::SubSys::UNITTEST));
            for (i = 0; i < numJobs; ++i) {
                Job* job = jobSys.CreateJobAsChild(root, &EmptyJob JOB_SYS_SUB_ARG(core::profiler::SubSys::UNITTEST));
                jobSys.Run(job);
            }

            addTime = timer.GetTimeVal();

            jobSys.Run(root);
            jobSys.Wait(root);

            runTime = timer.GetTimeVal();
        }

        times[n] = runTime.GetMilliSeconds();

        X_LOG0("JobSystem", "%" PRIuS " empty jobs: %gms addTime: %gms", numJobs, runTime.GetMilliSeconds(), addTime.GetMilliSeconds());
    }

    float total = core::accumulate(times.begin(), times.end(), 0.f);
    float avg = total / times.size();

    X_LOG0("JobSystem", "Avg time: %gms", avg);

    jobSys.ShutDown();
}

TEST(Threading, JobSystem2Empty_parallel_data)
{
    size_t count = 4000;

    JobSystem jobSys(g_arena);
    jobSys.StartUp();

    core::StopWatch timer;

    const size_t numJobs = 4000;
    const size_t numRuns = 10;

    std::array<float, numRuns> times;

    for (size_t n = 0; n < numRuns; n++) {
        core::TimeVal addTime;
        core::TimeVal runTime;
        {
            timer.Start();

            Goat* pData = nullptr;

            Job* root = jobSys.parallel_for(
                pData,
                count,
                &UpdateGoat,
                core::V2::CountSplitter(1)
                    JOB_SYS_SUB_ARG(core::profiler::SubSys::UNITTEST));

            addTime = timer.GetTimeVal();

            jobSys.Run(root);
            jobSys.Wait(root);

            runTime = timer.GetTimeVal();
        }
        times[n] = runTime.GetMilliSeconds();

        X_LOG0("JobSystem", "%" PRIuS " empty jobs: %gms addTime: %gms", numJobs, runTime.GetMilliSeconds(), addTime.GetMilliSeconds());
    }

    float total = core::accumulate(times.begin(), times.end(), 0.f);
    float avg = total / times.size();

    X_LOG0("JobSystem", "Avg time: %gms", avg);

    jobSys.ShutDown();
}

namespace NoData
{
    static void UpdateParticles(uintptr_t count)
    {
    }

    static void parallel_for_job(JobSystem& jobSys, size_t threadIdx, Job* pJob, void* pArg)
    {
        uintptr_t count = union_cast<uintptr_t, void*>(pArg);

        if (count > 2) {
            // split in two
            const uintptr_t leftCount = count / 2u;
            const uintptr_t rightCount = count - leftCount;

            Job* left = jobSys.CreateJobAsChild(pJob, parallel_for_job,
                union_cast<void*, uintptr_t>(leftCount) JOB_SYS_SUB_ARG(core::profiler::SubSys::UNITTEST));

            jobSys.Run(left);

            Job* right = jobSys.CreateJobAsChild(pJob, parallel_for_job,
                union_cast<void*, uintptr_t>(rightCount) JOB_SYS_SUB_ARG(core::profiler::SubSys::UNITTEST));

            jobSys.Run(right);
        }
        else {
            // execute the function on the range of data
            UpdateParticles(count);
        }
    }

} // namespace NoData

TEST(Threading, JobSystem2Empty_parallel)
{
    const size_t numJobs = 4000;

    JobSystem jobSys(g_arena);
    jobSys.StartUp();

    core::TimeVal MultiElapsed;
    core::StopWatch timer;
    {
        timer.Start();

        Job* root = jobSys.CreateJob(&NoData::parallel_for_job, reinterpret_cast<void*>(numJobs) JOB_SYS_SUB_ARG(core::profiler::SubSys::UNITTEST));
        jobSys.Run(root);
        jobSys.Wait(root);

        MultiElapsed = timer.GetTimeVal();
    }

    X_LOG0("JobSystem", "%" PRIuS " empty jobs: %gms", numJobs, MultiElapsed.GetMilliSeconds());
    jobSys.ShutDown();
}

TEST(Threading, JobSystem2Empty_parallel_for)
{
    JobSystem jobSys(g_arena);
    jobSys.StartUp();

    numJobsRan = 0;

    core::TimeVal MultiElapsed;
    core::StopWatch timer;
    {
        timer.Start();

        uint32_t* pCamels = nullptr;

        Job* job = jobSys.parallel_for(pCamels, 100000,
            &UpdateCamel, DataSizeSplitter(1024) JOB_SYS_SUB_ARG(core::profiler::SubSys::UNITTEST));

        jobSys.Run(job);
        jobSys.Wait(job);

        MultiElapsed = timer.GetTimeVal();
    }

    X_LOG0("JobSystem", "parallel_for: 100000 -> %i jobs %gms",
        numJobsRan, MultiElapsed.GetMilliSeconds());
    jobSys.ShutDown();
}

namespace Member
{
    class JobClass
    {
    public:
        void job(JobSystem& jobSys, size_t threadIdx, Job* pJob, void* pData)
        {
            X_UNUSED(jobSys);
            X_UNUSED(threadIdx);
            X_UNUSED(pJob);

            ++callCount_;
        }

        int32_t GetCallCount(void) const
        {
            return callCount_;
        }

    private:
        core::AtomicInt callCount_;
    };

} // namespace Member

TEST(Threading, JobSystem2Empty_member_func)
{
    JobSystem jobSys(g_arena);
    jobSys.StartUp();

    core::TimeVal MultiElapsed;
    core::StopWatch timer;
    {
        timer.Start();

        Member::JobClass inst;

        Job* job = jobSys.CreateMemberJob<Member::JobClass>(&inst, &Member::JobClass::job, nullptr JOB_SYS_SUB_ARG(core::profiler::SubSys::UNITTEST));

        jobSys.Run(job);
        jobSys.Wait(job);

        EXPECT_EQ(1, inst.GetCallCount());

        MultiElapsed = timer.GetTimeVal();
    }

    X_LOG0("JobSystem", "Member Function %gms", MultiElapsed.GetMilliSeconds());

    jobSys.ShutDown();
}

TEST(Threading, JobSystem2Empty_continuations)
{
    JobSystem jobSys(g_arena);
    jobSys.StartUp();

    core::TimeVal MultiElapsed;
    core::StopWatch timer;
    {
        timer.Start();

        Member::JobClass inst;

        Job* pSyncJob = jobSys.CreateEmtpyJob(JOB_SYS_SUB_ARG_SINGLE(core::profiler::SubSys::UNITTEST));

        Job* pJob = jobSys.CreateMemberJobAsChild<Member::JobClass>(pSyncJob, &inst, &Member::JobClass::job, nullptr JOB_SYS_SUB_ARG(core::profiler::SubSys::UNITTEST));

        // adds jobs that are run after the job above finished.
        // they are also creatd as childs so the parent job waits for them.
        jobSys.AddContinuation(pJob, jobSys.CreateMemberJobAsChild<Member::JobClass>(pSyncJob, &inst, &Member::JobClass::job, nullptr JOB_SYS_SUB_ARG(core::profiler::SubSys::UNITTEST)));
        jobSys.AddContinuation(pJob, jobSys.CreateMemberJobAsChild<Member::JobClass>(pSyncJob, &inst, &Member::JobClass::job, nullptr JOB_SYS_SUB_ARG(core::profiler::SubSys::UNITTEST)));
        jobSys.AddContinuation(pJob, jobSys.CreateMemberJobAsChild<Member::JobClass>(pSyncJob, &inst, &Member::JobClass::job, nullptr JOB_SYS_SUB_ARG(core::profiler::SubSys::UNITTEST)));
        jobSys.AddContinuation(pJob, jobSys.CreateMemberJobAsChild<Member::JobClass>(pSyncJob, &inst, &Member::JobClass::job, nullptr JOB_SYS_SUB_ARG(core::profiler::SubSys::UNITTEST)), true);
        jobSys.AddContinuation(pJob, jobSys.CreateMemberJobAsChild<Member::JobClass>(pSyncJob, &inst, &Member::JobClass::job, nullptr JOB_SYS_SUB_ARG(core::profiler::SubSys::UNITTEST)));
        jobSys.AddContinuation(pJob, jobSys.CreateMemberJobAsChild<Member::JobClass>(pSyncJob, &inst, &Member::JobClass::job, nullptr JOB_SYS_SUB_ARG(core::profiler::SubSys::UNITTEST)));

        jobSys.AddContinuation(pJob, jobSys.CreateMemberJobAsChild<Member::JobClass>(pSyncJob, &inst, &Member::JobClass::job, nullptr JOB_SYS_SUB_ARG(core::profiler::SubSys::UNITTEST)));

        // if i make a job and it has child jobs
        // how do i wait for them all to finish?
        jobSys.Run(pJob);

        jobSys.Run(pSyncJob);
        jobSys.Wait(pSyncJob);

        EXPECT_EQ(8, inst.GetCallCount());

        MultiElapsed = timer.GetTimeVal();
    }

    X_LOG0("JobSystem", "Member Function %gms", MultiElapsed.GetMilliSeconds());

    jobSys.ShutDown();
}
