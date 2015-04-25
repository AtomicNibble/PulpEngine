#include "stdafx.h"

#include "Threading\JobList.h"
#include <Memory\ThreadPolicies\MultiThreadPolicy.h>
#include <Threading\CriticalSection.h>

#include "gtest/gtest.h"

#include "Profiler.h"

#include <ITimer.h>

X_USING_NAMESPACE;

using namespace core::JobList;

core::AtomicInt numJobsRan(0);
core::AtomicInt jobList9Done(0);

X_PRAGMA(optimize("", off))

void TestJob(void* pParam, uint32_t batchOffset, uint32_t batchNum, uint32_t workerIdx)
{
	uint32_t idx = reinterpret_cast<uint32_t>(pParam);

	size_t i, running_total = 23;
	size_t some_large_number = 0x10000;
	for (i = 0; i < some_large_number; i++)
	{
		running_total = 37 * running_total + i;
	}

	size_t jobIdx = idx / 10000;

	if (jobIdx == 9)  {
		ASSERT_EQ(1, jobList9Done);
	}

	if (idx == 80999) {
		jobList9Done = 1;
	}

//	X_LOG0("TestJob", "jobList idx: %i", jobIdx);

	++numJobsRan;
}
X_PRAGMA(optimize("", on))


TEST(Threading, JobList)
{
	jobListRunner jobSys;

	jobSys.StartUp();

	core::TimeVal singleThreadElapse;
	{
		core::TimeVal start = gEnv->pTimer->GetTimeReal();

		size_t i;
		size_t num = 10000;
		for (i = 0; i < num; i++)
		{
			TestJob((void*)i, 0, 1, 0);
		}

		core::TimeVal end = gEnv->pTimer->GetTimeReal();
		singleThreadElapse = end - start;

		X_LOG0("Scheduler", "Single threaded exec time: %f", singleThreadElapse.GetMilliSeconds());
	}


	{
		core::MallocFreeAllocator allocator;
		typedef core::MemoryArena<
			core::MallocFreeAllocator,
			core::MultiThreadPolicy<core::CriticalSection>,
			core::SimpleBoundsChecking,
			core::SimpleMemoryTracking,
			core::SimpleMemoryTagging
		> StackArena;

		StackArena arena(&allocator, "SchedulerArena");

		core::TimeVal start = gEnv->pTimer->GetTimeReal();

		const size_t numLists = 10;
		JobListStats combinedStats;

		JobList* jobLists[numLists];
		for (size_t j = 0; j < numLists; j++)
		{
			jobLists[j] = X_NEW(JobList, &arena, "JobLists")(&arena);
		}

		for (size_t p = 0; p < 1; p++)
		{

			for (size_t j = 0; j < numLists; j++)
			{
				JobList* jobs = jobLists[j];

				jobs->listId_ = j;

				for (size_t i = 0; i < 1000; i++) {
					jobs->AddJob(TestJob, (void*)(i + (10000 * j)));
				}

				// set a priority.
				if (j == (numLists - 2)) {
					jobs->SetPriority(JobListPriority::LOW);
				}
				if ((j % 3) == 0 || j == (numLists - 1)) {
					jobs->SetPriority(JobListPriority::HIGH);
				}
				if (j == 0) {
					jobs->SetPriority(JobListPriority::LOW);
				}
			}

			for (size_t j = 0; j < numLists; j++)
			{
				if (j == (numLists-1))
					jobSys.SubmitJobList(jobLists[j], jobLists[j-1]);
				else
					jobSys.SubmitJobList(jobLists[j]);
			}

			for (size_t j = 0; j < numLists; j++)
			{
#if SCHEDULER_LOGS
				X_LOG0("Scheduler", "wait for list: %i", j);
#endif // !SCHEDULER_LOGS
				jobLists[j]->Wait();
			}

			X_LOG0_EVERY_N(10, "Scheduler", "JobList run number: %i numJobsDone: %i", p, numJobsRan);
		}


		jobSys.WaitForThreads();

		for (size_t j = 0; j < numLists; j++)
		{
			const JobListStats& stats = jobLists[j]->getStats();
			combinedStats.waitTime += stats.waitTime;
			for (size_t x = 0; x < HW_THREAD_MAX; x++) {
				combinedStats.threadExecTime[x] += stats.threadExecTime[x];
				combinedStats.threadTotalTime[x] += stats.threadTotalTime[x];
			}
			X_DELETE(jobLists[j], &arena);
		}

		core::TimeVal end = gEnv->pTimer->GetTimeReal();
		core::TimeVal MultiElapsed = end - start;

		// work out percentage.
		// if it took 5 times less time it is 500%
		float32_t percentage = static_cast<float32_t>(singleThreadElapse.GetValue()) /
			static_cast<float32_t>(MultiElapsed.GetValue());

		percentage *= 100;

		// print the stats.
		X_LOG0("Scheduler", "Stats");
		X_LOG_BULLET;
		X_LOG0("Scheduler", "Percentage: %g%% scaling: %g%%", percentage, percentage / jobSys.numThreads());
		X_LOG0("Scheduler", "Total wait time: %f", combinedStats.waitTime.GetMilliSeconds());
		for (size_t i = 0; i < HW_THREAD_MAX; i++) {
			X_LOG0("Scheduler", "Thread %i Exec: %f Total: %f", 
				i,
				combinedStats.threadExecTime[i].GetMilliSeconds(),
				combinedStats.threadTotalTime[i].GetMilliSeconds()
				);
		}
	}

	EXPECT_EQ(20000, numJobsRan);

	jobSys.ShutDown();
}

