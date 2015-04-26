#include "stdafx.h"

#include "Threading\JobSystem.h"

#include "gtest/gtest.h"


X_USING_NAMESPACE;

using namespace core;

namespace
{
	core::AtomicInt numJobsRan(0);

	X_PRAGMA(optimize("", off))
	void TestJob(void* pParam, uint32_t workerIdx)
	{
		uint32_t idx = reinterpret_cast<uint32_t>(pParam);

		size_t i, running_total = 23;
		size_t some_large_number = 0x8000;
		for (i = 0; i < some_large_number; i++)
		{
			running_total = 37 * running_total + i;
		}

		// X_LOG0("TestJob", "job idx: %i threadIdx: %i", idx, workerIdx);

		++numJobsRan;
	}
	X_PRAGMA(optimize("", on))

}



TEST(Threading, JobSystem)
{
	JobSystem jobSys;

	jobSys.StartUp();

	// create jobs on stack.
	JobDecl jobs[0x100];
	size_t i;

	for (i = 0; i < 0x100; i++)
	{
		jobs[i] = JobDecl(TestJob, (void*)(i));
	}


	jobSys.AddJobs(jobs, 0x80, JobPriority::NORMAL);
	jobSys.AddJobs(&jobs[0x80], 0x80, JobPriority::HIGH);

	jobSys.waitForAllJobs();

	EXPECT_EQ(0x100, numJobsRan);

	jobSys.ShutDown();
}