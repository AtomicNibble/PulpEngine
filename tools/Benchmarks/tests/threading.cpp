#include "stdafx.h"

#include <Threading\JobSystem2.h>


namespace
{

	void EmptyJob(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* job, void* pParam)
	{
	}

	void EmptyJob(char*, size_t count)
	{

	}

} // namespace



void BM_threading_jobsys_create(benchmark::State& state)
{
	core::V2::JobSystem jobSys(g_arena);
	jobSys.StartUp();

	const int32_t numJobs = core::V2::JobSystem::MAX_JOBS - 1;
	char* pData = nullptr;

	while (state.KeepRunning()) {

		auto* root = jobSys.parallel_for(
			pData,
			numJobs,
			&EmptyJob,
			core::V2::CountSplitter(1)
			JOB_SYS_SUB_ARG(core::profiler::SubSys::UNITTEST)
		);

		jobSys.Run(root);
		jobSys.Wait(root);
	}

	state.SetItemsProcessed(numJobs * state.iterations());
	
	jobSys.ShutDown();
}

BENCHMARK(BM_threading_jobsys_create)->Unit(benchmark::kMicrosecond);
