#include "stdafx.h"

#include "Threading\Scheduler.h"

#include "gtest/gtest.h"

#include "Profiler.h"

X_USING_NAMESPACE;


void TestJob(void* pParam, uint32_t batchOffset, uint32_t batchNum, uint32_t workerIdx)
{
	uint32_t idx = reinterpret_cast<uint32_t>(pParam);

	X_LOG0("TestJob", "job idx: %i Worker: %i", idx, workerIdx);
	Sleep(10);
}


TEST(Threading, Scheduler)
{
	core::Scheduler jobSys;
	core::JobList jobLists[4];

	jobSys.StartThreads();

	{
		UnitTests::ScopeProfiler profile("Scheduler");

			for(size_t j=0;j<4; j++)
			{
				core::JobList& jobs = jobLists[j];

				for (size_t i = 0; i < 100; i++) {
					jobs.AddJob(TestJob, (void*)i + (j* 1000));
				}

				// set a priority.
				if(j == 1) {
					jobs.SetPriority(JobListPriority::HIGH);
				}
				if(j == 2) {
					jobs.SetPriority(JobListPriority::NONE);
				}
				if(j == 3) {
					jobs.SetPriority(JobListPriority::LOW);
				}

			}

			for(size_t j=0;j<4; j++)
			{
				jobSys.SubmitJobList(&jobLists[j]);
			}

			for(size_t j=0;j<4; j++)
			{
				jobLists[j].Wait();
			}	
	}

	jobSys.ShutDown();
}