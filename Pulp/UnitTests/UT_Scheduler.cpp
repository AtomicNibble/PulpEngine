#include "stdafx.h"

#include "Threading\Scheduler.h"

#include "gtest/gtest.h"

#include "Profiler.h"

X_USING_NAMESPACE;


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

//	X_LOG0("TestJob", "job idx: %i Worker: %i result: %p", idx, workerIdx, running_total);
}


TEST(Threading, DISABLED_Scheduler)
{
	core::Scheduler jobSys;
	core::JobList jobLists[4];

	jobSys.StartThreads();

	{
		UnitTests::ScopeProfiler profile("Scheduler");

		for (size_t j = 0; j < 4; j++)
		{
			core::JobList& jobs = jobLists[j];

			for (size_t i = 0; i < 100; i++) {
				jobs.AddJob(TestJob, (void*)(i + (j * 1000)));
			}

			// set a priority.
			if (j == 1) {
				jobs.SetPriority(core::JobListPriority::HIGH);
			}
			if (j == 0) {
				jobs.SetPriority(core::JobListPriority::LOW);
			}
		}

		for (size_t i = 0; i < 0x100; i++)
		{
			for (size_t j = 0; j < 4; j++)
			{
				jobSys.SubmitJobList(&jobLists[j]);
			}

			for (size_t j = 0; j < 4; j++)
			{
				jobLists[j].Wait();
			}
		}
	}

	jobSys.ShutDown();
}