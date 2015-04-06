#include "stdafx.h"

#include "Threading\Scheduler.h"
#include <Memory\ThreadPolicies\MultiThreadPolicy.h>
#include <Threading\CriticalSection.h>

#include "gtest/gtest.h"

#include "Profiler.h"

#include <ITimer.h>

X_USING_NAMESPACE;

core::AtomicInt numJobsRan(0);

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

	++numJobsRan;
}
X_PRAGMA(optimize("", on))


TEST(Threading, Scheduler)
{
	core::Scheduler jobSys;

	jobSys.StartThreads();

#if 0 // test how long the singe threaded version took.
	{
		core::TimeVal start = gEnv->pTimer->GetTimeReal();

		size_t i;
		size_t num = 10000;
		for (i = 0; i < num; i++)
		{
			TestJob((void*)i, 0, 1, 0);

			if ((i % 100000) == 0) {
				X_LOG0("Test", "left: %i", num - i);
			}
		}

		core::TimeVal end = gEnv->pTimer->GetTimeReal();
		core::TimeVal elpased = end - start;

		X_LOG0("Test", "exec time: %f", elpased.GetMilliSeconds());
	}
#endif

	{
		UnitTests::ScopeProfiler profile("Scheduler");

		core::MallocFreeAllocator allocator;
		typedef core::MemoryArena<
			core::MallocFreeAllocator,
			core::MultiThreadPolicy<core::CriticalSection>,
			core::SimpleBoundsChecking,
			core::SimpleMemoryTracking,
			core::SimpleMemoryTagging
		> StackArena;

		StackArena arena(&allocator, "SchedulerArena");

		const size_t numLists = 10;

		core::JobListStats combinedStats;

		core::JobList* jobLists[numLists];
		for (size_t j = 0; j < numLists; j++)
		{
			jobLists[j] = X_NEW(core::JobList, &arena, "JobLists")(&arena);
		}

		for (size_t p = 0; p < 1; p++)
		{

			for (size_t j = 0; j < numLists; j++)
			{
				core::JobList* jobs = jobLists[j];

				jobs->listId_ = j;

				for (size_t i = 0; i < 1000; i++) {
					jobs->AddJob(TestJob, (void*)(i + (10000 * j)));
				}

				// set a priority.
				if ((j % 3) == 0) {
					jobs->SetPriority(core::JobListPriority::HIGH);
				}
				if (j == 0) {
					jobs->SetPriority(core::JobListPriority::LOW);
				}
			}

			for (size_t j = 0; j < numLists; j++)
			{
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
			combinedStats += jobLists[j]->getStats();
			X_DELETE(jobLists[j], &arena);
		}


		// print the stats.
		X_LOG0("Scheduler", "Stats");
		X_LOG_BULLET;

		X_LOG0("Scheduler", "Total wait time: %f", combinedStats.waitTime.GetMilliSeconds());
		for (size_t i = 0; i < core::HW_THREAD_MAX; i++) {
			X_LOG0("Scheduler", "Thread %i Exec: %f Total: %f", 
				i,
				combinedStats.threadExecTime[i].GetMilliSeconds(),
				combinedStats.threadTotalTime[i].GetMilliSeconds()
				);
		}
	}

	EXPECT_EQ(10000, numJobsRan);

	jobSys.ShutDown();
}

