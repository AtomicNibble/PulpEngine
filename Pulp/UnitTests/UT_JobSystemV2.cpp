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


} // namespace



TEST(Threading, JobSystem2Empty)
{
	JobSystem jobSys;
	jobSys.StartUp();

	const size_t numJobs = 4000;

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

	X_LOG0("JobSystem", "%" PRIuS " empty jobs: %gms", numJobs, MultiElapsed.GetMilliSeconds());

	jobSys.ShutDown();
}


namespace Data
{
	struct Goat
	{
		int32_t goat;
		int32_t boat;
	};

	struct parallel_for_job_data
	{
		parallel_for_job_data(Goat* data,
			unsigned int count,
			void(*function)(Goat*, unsigned int)) :
			data(data), count(count), function(function)
		{

		}

		Goat* data;
		unsigned int count;
		void(*function)(Goat*, unsigned int);
	};

	void UpdateGoat(Goat*, unsigned int count)
	{

	}

	static void parallel_for_job(JobSystem& jobSys, size_t threadIdx, Job* pJob, void* pArg)
	{
		const parallel_for_job_data* data = static_cast<const parallel_for_job_data*>(pArg);

		if (data->count > 2)
		{
			// split in two
			const unsigned int leftCount = data->count / 2u;
			const unsigned int rightCount = data->count - leftCount;

			const parallel_for_job_data leftData(data->data, leftCount, data->function);
			Job* left = jobSys.CreateJobAsChild(pJob, parallel_for_job, leftData);
			jobSys.Run(left);

			const parallel_for_job_data rightData(data->data, leftCount, data->function);
			Job* right = jobSys.CreateJobAsChild(pJob, parallel_for_job, rightData);
			jobSys.Run(right);
		}
		else
		{
			// execute the function on the range of data
			(data->function)(data->data, data->count);
		}
	}

} // namespace Data

TEST(Threading, JobSystem2Empty_parallel_data)
{
	unsigned int count = 4000;

	JobSystem jobSys;
	jobSys.StartUp();

	core::TimeVal MultiElapsed;
	core::StopWatch timer;
	{
		timer.Start();
		
		Data::Goat* data = nullptr;

		const Data::parallel_for_job_data jobData = { data, count, Data::UpdateGoat };
		Job* root = jobSys.CreateJob(&Data::parallel_for_job, jobData);
		jobSys.Run(root);
		jobSys.Wait(root);

		MultiElapsed = timer.GetTimeVal();
	}

	X_LOG0("JobSystem", "count: %i elapsed: %gms", count, MultiElapsed.GetMilliSeconds());
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

		if (count > 2)
		{
			// split in two
			const uintptr_t leftCount = count / 2u;
			const uintptr_t rightCount = count - leftCount;


			Job* left = jobSys.CreateJobAsChild(pJob, parallel_for_job,
				union_cast<void*, uintptr_t>(leftCount));

			jobSys.Run(left);

			Job* right = jobSys.CreateJobAsChild(pJob, parallel_for_job,
				union_cast<void*, uintptr_t>(rightCount));

			jobSys.Run(right);
		}
		else
		{
			// execute the function on the range of data
			UpdateParticles(count);
		}
	}

} // namespace NoData

TEST(Threading, JobSystem2Empty_parallel)
{
	const size_t numJobs = 4000;

	JobSystem jobSys;
	jobSys.StartUp();

	core::TimeVal MultiElapsed;
	core::StopWatch timer;
	{
		timer.Start();

		Job* root = jobSys.CreateJob(&NoData::parallel_for_job, reinterpret_cast<void*>(numJobs));
		jobSys.Run(root);
		jobSys.Wait(root);

		MultiElapsed = timer.GetTimeVal();
	}

	X_LOG0("JobSystem", "%" PRIuS " empty jobs: %gms", numJobs, MultiElapsed.GetMilliSeconds());
	jobSys.ShutDown();
}


TEST(Threading, JobSystem2Empty_parallel_for)
{
	JobSystem jobSys;
	jobSys.StartUp();

	numJobsRan = 0;

	core::TimeVal MultiElapsed;
	core::StopWatch timer;
	{
		timer.Start();

		uint32_t* pCamels = nullptr;

		Job* job = jobSys.parallel_for(pCamels, 100000,
			&UpdateCamel, DataSizeSplitter(1024));

		jobSys.Run(job);
		jobSys.Wait(job);

		MultiElapsed = timer.GetTimeVal();
	}

	X_LOG0("JobSystem", "parallel_for: 100000 -> %i jobs %gms",
		numJobsRan,  MultiElapsed.GetMilliSeconds());
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

		int32_t GetCallCount(void) const {
			return callCount_;
		}

	private:
		core::AtomicInt callCount_;
	};

} // namespace Member

TEST(Threading, JobSystem2Empty_member_func)
{
	JobSystem jobSys;
	jobSys.StartUp();


	core::TimeVal MultiElapsed;
	core::StopWatch timer;
	{
		timer.Start();

		Member::JobClass inst;

		Job* job = jobSys.CreateMemberJob<Member::JobClass>(&inst, &Member::JobClass::job, nullptr);

		jobSys.Run(job);
		jobSys.Wait(job);

		EXPECT_EQ(1, inst.GetCallCount());

		MultiElapsed = timer.GetTimeVal();
	}

	X_LOG0("JobSystem", "Member Function %gms",  MultiElapsed.GetMilliSeconds());

	jobSys.ShutDown();
}



TEST(Threading, JobSystem2Empty_continuations)
{
	JobSystem jobSys;
	jobSys.StartUp();


	core::TimeVal MultiElapsed;
	core::StopWatch timer;
	{
		timer.Start();

		Member::JobClass inst;

		Job* pJob = jobSys.CreateMemberJob<Member::JobClass>(&inst, &Member::JobClass::job, nullptr);

		jobSys.AddContinuation(pJob, jobSys.CreateMemberJobAsChild<Member::JobClass>(pJob, &inst, &Member::JobClass::job, nullptr));
		jobSys.AddContinuation(pJob, jobSys.CreateMemberJobAsChild<Member::JobClass>(pJob, &inst, &Member::JobClass::job, nullptr));
		jobSys.AddContinuation(pJob, jobSys.CreateMemberJobAsChild<Member::JobClass>(pJob, &inst, &Member::JobClass::job, nullptr));
		jobSys.AddContinuation(pJob, jobSys.CreateMemberJobAsChild<Member::JobClass>(pJob, &inst, &Member::JobClass::job, nullptr), true);
		jobSys.AddContinuation(pJob, jobSys.CreateMemberJobAsChild<Member::JobClass>(pJob, &inst, &Member::JobClass::job, nullptr));
		jobSys.AddContinuation(pJob, jobSys.CreateMemberJobAsChild<Member::JobClass>(pJob, &inst, &Member::JobClass::job, nullptr));
	
		jobSys.AddContinuation(pJob, jobSys.CreateMemberJobAsChild<Member::JobClass>(pJob, &inst, &Member::JobClass::job, nullptr));

		// if i make a job and it has child jobs
		// how do i wait for them all to finish?

		jobSys.Run(pJob);
		jobSys.Wait(pJob);

		EXPECT_EQ(8, inst.GetCallCount());

		MultiElapsed = timer.GetTimeVal();
	}

	X_LOG0("JobSystem", "Member Function %gms", MultiElapsed.GetMilliSeconds());

	jobSys.ShutDown();
}



