#include "stdafx.h"

#include "Threading\Scheduler.h"
#include <Memory\ThreadPolicies\MultiThreadPolicy.h>
#include <Threading\CriticalSection.h>

#include "gtest/gtest.h"

#include "Profiler.h"

X_USING_NAMESPACE;

core::AtomicInt numJobsRan(0);

void TestJob(void* pParam, uint32_t batchOffset, uint32_t batchNum, uint32_t workerIdx)
{
	uint32_t idx = reinterpret_cast<uint32_t>(pParam);

//	Sleep(10);

	size_t i, running_total = 23;
	size_t some_large_number = 0x10000 * (workerIdx+1);
	for (i = 0; i < some_large_number; i++)
	{
		running_total = 37 * running_total + i;
	}

	if ((idx % 10000 ) == 99) {
#if SCHEDULER_LOGS
		X_LOG0("TestJob", "last job: pParam: 0%i worker: 0%x", idx, workerIdx);
#endif // !SCHEDULER_LOGS
	}

	++numJobsRan;

//	X_LOG0("TestJob", "job idx: %i Worker: %i result: %p", idx, workerIdx, running_total);
}


TEST(Threading, Scheduler)
{
	core::Scheduler jobSys;

	jobSys.StartThreads();

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

		for (size_t p = 0; p < 0x4000; p++)
		{
			core::JobList* jobLists[numLists];
			for (size_t j = 0; j < numLists; j++)
			{
				jobLists[j] = X_NEW(core::JobList, &arena, "JobLists")(&arena);
			}

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

			jobSys.WaitForThreads();

			for (size_t j = 0; j < numLists; j++)
			{
				combinedStats += jobLists[j]->getStats();
				X_DELETE(jobLists[j], &arena);
			}

			X_LOG0_EVERY_N(100, "Scheduler", "JobList run number: %i numJobsDone: %i", p, numJobsRan);
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

	EXPECT_EQ(2560000, numJobsRan);

	jobSys.ShutDown();
}

