#include "stdafx.h"

#include "Threading\JobSystem2.h"
#include "Util\StopWatch.h"
#include "Time\TimeVal.h"


#include "gtest/gtest.h"

X_USING_NAMESPACE;


using namespace core;
using namespace core::V2;

namespace
{
	core::AtomicInt numJobsRan(0);

	static void EmptyJob(JobSystem* pJobSys, size_t threadIdx, Job* job, void* pParam)
	{
//		++numJobsRan;

	}
}



TEST(Threading, JobSystem2Empty)
{
	JobSystem jobSys;
	jobSys.Start();

	const size_t numJobs = 65000;

	core::TimeVal MultiElapsed;
	core::StopWatch timer;
	{
		timer.Start();

		size_t i;

		Job* root = jobSys.CreateJob(&EmptyJob);
		for (i = 0; i < numJobs; ++i)
		{
			Job* job = jobSys.CreateJobAsChild(root, &EmptyJob);
			jobSys.Run(job);
		}

		jobSys.Run(root);
		jobSys.Wait(root);

		MultiElapsed = timer.GetTimeVal();
	}

	X_LOG0("JobSystem", "%i empty jobs: %gms", numJobs, MultiElapsed.GetMilliSeconds());

	jobSys.ShutDown();
}


namespace
{
	void UpdateParticles(uintptr_t count)
	{

	}

	static void parallel_for_job(JobSystem* pJobSys, size_t threadIdx, Job* pJob, void* pArg)
	{
		uintptr_t count = union_cast<uintptr_t, void*>(pArg);

		if (count > 2)
		{
			// split in two
			const uintptr_t leftCount = count / 2u;
			const uintptr_t rightCount = count - leftCount;


			Job* left = pJobSys->CreateJobAsChild(pJob, parallel_for_job,
				union_cast<void*, uintptr_t>(leftCount));

			pJobSys->Run(left);

			Job* right = pJobSys->CreateJobAsChild(pJob, parallel_for_job,
				union_cast<void*, uintptr_t>(rightCount));
		
			pJobSys->Run(right);
		}
		else
		{
			// execute the function on the range of data
			UpdateParticles(count);
		}
	}



}

TEST(Threading, JobSystem2Empty_parallel)
{
	const size_t numJobs = 65000;

	JobSystem jobSys;
	jobSys.Start();

	core::TimeVal MultiElapsed;
	core::StopWatch timer;
	{
		timer.Start();

		Job* root = jobSys.CreateJob(&parallel_for_job, reinterpret_cast<void*>(numJobs));
		jobSys.Run(root);
		jobSys.Wait(root);

		MultiElapsed = timer.GetTimeVal();
	}

	X_LOG0("JobSystem", "%i empty jobs: %gms", numJobs, MultiElapsed.GetMilliSeconds());
	jobSys.ShutDown();
}