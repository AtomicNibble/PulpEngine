#include "stdafx.h"

#include "Threading\Scheduler.h"

#include "gtest/gtest.h"

X_USING_NAMESPACE;


void TestJob(void* pParam, uint32_t batchOffset, uint32_t batchNum, uint32_t workerIdx)
{
	uint32_t idx = reinterpret_cast<uint32_t>(pParam);

	X_LOG0("TestJob", "job idx: %i Worker: %i", idx, workerIdx);
	Sleep(150);
}


TEST(Threading, Scheduler)
{
	core::Scheduler jobSys;
	core::JobList jobs;

	jobSys.StartThreads();

	{
		for (size_t i = 0; i < 100; i++) {
			jobs.AddJob(TestJob, (void*)i);
		}

		jobSys.SubmitJobList(&jobs);


		jobs.Wait();
	}

	::Sleep(60000);
	jobSys.ShutDown();
}