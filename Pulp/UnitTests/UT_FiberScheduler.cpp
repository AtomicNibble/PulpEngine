#include "stdafx.h"

#include "Threading\FiberScheduler.h"
#include "Time\StopWatch.h"
#include "Time\TimeVal.h"

X_USING_NAMESPACE;

using namespace core;

namespace
{
    X_PRAGMA(optimize("", off))

    X_ALIGNED_SYMBOL(struct NumberSubset, 128) // prevent false sharing.
    {
        uint64 start;
        uint64 end;
        uint64 total;
    };

    core::AtomicInt numJobsRan(0);

    void AddNumberSubset(Fiber::Scheduler* pScheduler, void* pArg)
    {
        X_UNUSED(pScheduler);
        NumberSubset* pSubSet = reinterpret_cast<NumberSubset*>(pArg);
        NumberSubset localSet = *pSubSet;

        localSet.total = 0;

        while (localSet.start != localSet.end) {
            localSet.total += localSet.start;
            ++localSet.start;
        }

        localSet.total += localSet.end;

        *pSubSet = localSet;
        ++numJobsRan;
    }

    X_PRAGMA(optimize("", on))
} // namespace

TEST(Threading, FiberScheduler)
{
    // Define the constants to test
    const uint64 triangleNum = 47593243ull;
    const uint64 numAdditionsPerTask = 10000ull;
    const uint64 numTasks = (triangleNum + numAdditionsPerTask - 1ull) / numAdditionsPerTask;
    const uint64 expectedValue = triangleNum * (triangleNum + 1ull) / 2ull;

    Fiber::Task* pTasks = X_NEW_ARRAY(Fiber::Task, numTasks, g_arena, "FiberTask");
    NumberSubset* pSubSets = X_NEW_ARRAY(NumberSubset, numTasks, g_arena, "FiberTaskData");

    uint64 nextNumber = 1ull;
    for (uint64 i = 0ull; i < numTasks; ++i) {
        NumberSubset* subset = &pSubSets[i];

        subset->start = nextNumber;
        subset->end = nextNumber + numAdditionsPerTask - 1ull;
        if (subset->end > triangleNum) {
            subset->end = triangleNum;
        }

        pTasks[i] = {AddNumberSubset, subset};

        nextNumber = subset->end + 1;
    }

    core::TimeVal singleThreadElapse;
    {
        core::StopWatch timer;

        for (uint64 i = 0ull; i < numTasks; ++i) {
            AddNumberSubset(nullptr, &pSubSets[i]);
        }

        // Add the results
        uint64 result = 0ull;
        for (uint64 i = 0; i < numTasks; ++i) {
            result += pSubSets[i].total;
        }

        singleThreadElapse = timer.GetTimeVal();

        EXPECT_EQ(expectedValue, result);
        EXPECT_EQ(numTasks, numJobsRan);
        X_LOG0("FiberScheduler", "Single threaded exec time: %f", singleThreadElapse.GetMilliSeconds());
    }

    nextNumber = 1ull;
    for (uint64 i = 0ull; i < numTasks; ++i) {
        NumberSubset* subset = &pSubSets[i];

        subset->start = nextNumber;
        subset->end = nextNumber + numAdditionsPerTask - 1ull;
        if (subset->end > triangleNum) {
            subset->end = triangleNum;
        }

        pTasks[i] = {AddNumberSubset, subset};
        nextNumber = subset->end + 1;
    }

    numJobsRan = 0;

    {
        Fiber::Scheduler scheduler;

        ASSERT_TRUE(scheduler.StartUp());

        core::TimeVal MultiElapsed;
        {
            core::Thread::setName(core::Thread::getCurrentID(), "MainThread");

            core::StopWatch timer;

            core::AtomicInt* pCounter = nullptr;
            scheduler.AddTasks(pTasks, numTasks, &pCounter);

            scheduler.WaitForCounterAndFree(pCounter, 0);

            MultiElapsed = timer.GetTimeVal();

            // Add the results
            uint64 result = 0ull;
            for (uint64 i = 0; i < numTasks; ++i) {
                result += pSubSets[i].total;
            }

            EXPECT_EQ(expectedValue, result);
            EXPECT_EQ(numTasks, numJobsRan);
        }

        X_DELETE_ARRAY(pTasks, g_arena);
        X_DELETE_ARRAY(pSubSets, g_arena);

        // work out percentage.
        // if it took 5 times less time it is 500%
        float32_t percentage = static_cast<float32_t>(singleThreadElapse.GetValue()) / static_cast<float32_t>(MultiElapsed.GetValue());

        percentage *= 100;

        // print the stats.
        X_LOG0("FiberScheduler", "Stats");
        X_LOG_BULLET;
        X_LOG0("FiberScheduler", "SingleThreaded: %g", singleThreadElapse.GetMilliSeconds());
        X_LOG0("FiberScheduler", "MultiThreaded: %g", MultiElapsed.GetMilliSeconds());
        X_LOG0("FiberScheduler", "Percentage: %g%% scaling: %g%%", percentage, percentage / scheduler.NumThreads());

        scheduler.ShutDown();
    }
}

namespace
{
    static void EmptyJob(Fiber::Scheduler* pScheduler, void* pArg)
    {
    }

    core::AtomicInt* gpCounter = nullptr;
    //	core::AtomicInt numJobsRanEmpty(0);

    void UpdateParticles(uintptr_t count)
    {
        //	++numJobsRanEmpty;
    }

    static void parallel_for_job(Fiber::Scheduler* pScheduler, void* pArg)
    {
        uintptr_t count = union_cast<uintptr_t, void*>(pArg);

        if (count > 2) {
            Fiber::Task task;
            task.Function = parallel_for_job;

            // split in two
            const uintptr_t leftCount = count / 2u;
            const uintptr_t rightCount = count - leftCount;

            task.pArgData = union_cast<void*, uintptr_t>(leftCount);
            pScheduler->AddTask(task, &gpCounter, Fiber::JobPriority::HIGH);

            task.pArgData = union_cast<void*, uintptr_t>(rightCount);
            pScheduler->AddTask(task, &gpCounter, Fiber::JobPriority::HIGH);
        }
        else {
            // execute the function on the range of data
            UpdateParticles(count);
        }
    }

} // namespace

TEST(Threading, FiberSchedulerEmpty)
{
    const size_t numJobs = 65000;

    Fiber::Scheduler scheduler;

    ASSERT_TRUE(scheduler.StartUp());

    core::AtomicInt* pCounter = nullptr;

    core::TimeVal MultiElapsed;
    core::StopWatch timer;
    {
        timer.Start();

        size_t i;
        for (i = 0; i < numJobs; i++) {
            Fiber::Task task;
            task.pArgData = 0;
            task.Function = EmptyJob;

            scheduler.AddTask(task, &pCounter, Fiber::JobPriority::HIGH);
        }

        scheduler.WaitForCounterAndFree(pCounter, 0);

        MultiElapsed = timer.GetTimeVal();
    }

    X_LOG0("FiberScheduler", "%i empty jobs: %gms", numJobs, MultiElapsed.GetMilliSeconds());

    scheduler.ShutDown();
}

TEST(Threading, FiberSchedulerEmpty_parallel)
{
    const size_t numJobs = 65000;

    Fiber::Scheduler scheduler;

    ASSERT_TRUE(scheduler.StartUp());

    core::TimeVal MultiElapsed;
    core::StopWatch timer;
    {
        timer.Start();

        Fiber::Task task;
        task.Function = parallel_for_job;
        task.pArgData = reinterpret_cast<void*>(numJobs);

        scheduler.AddTask(task, &gpCounter, Fiber::JobPriority::HIGH);
        scheduler.WaitForCounterAndFree(gpCounter, 0);

        MultiElapsed = timer.GetTimeVal();
    }

    X_LOG0("FiberScheduler", "%i empty jobs: %gms", numJobs, MultiElapsed.GetMilliSeconds());

    scheduler.ShutDown();
}