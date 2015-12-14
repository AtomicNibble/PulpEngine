#include "stdafx.h"

#include "Threading\FiberScheduler.h"
#include "Util\StopWatch.h"
#include "Time\TimeVal.h"

#include "gtest/gtest.h"

#include "Profiler.h"

X_USING_NAMESPACE;

using namespace core;

namespace
{
	X_PRAGMA(optimize("", off))

	X_ALIGNED_SYMBOL(struct NumberSubset, 128) // prevent false sharing.
	{
		uint64 start;
		uint64 end;
		uint64 total;
	};

	core::AtomicInt numJobsRan(0);

	void AddNumberSubset(Fiber::Scheduler* pScheduler, void* pArg)
	{
		X_UNUSED(pScheduler);
		NumberSubset* pSubSet = reinterpret_cast<NumberSubset*>(pArg);
		NumberSubset localSet = *pSubSet;

		localSet.total = 0;

		while (localSet.start != localSet.end) {
			localSet.total += localSet.start;
			++localSet.start;
		}

		localSet.total += localSet.end;

		*pSubSet = localSet;
		++numJobsRan;
	}

	X_PRAGMA(optimize("", on))
}



TEST(Threading, FiberScheduler)
{
	// Define the constants to test
	const uint64 triangleNum = 47593243ull;
	const uint64 numAdditionsPerTask = 10000ull;
	const uint64 numTasks = (triangleNum + numAdditionsPerTask - 1ull) / numAdditionsPerTask;
	const uint64 expectedValue = triangleNum * (triangleNum + 1ull) / 2ull;

	Fiber::Task* pTasks = X_NEW_ARRAY(Fiber::Task, numTasks, g_arena, "FiberTask");
	NumberSubset* pSubSets = X_NEW_ARRAY(NumberSubset, numTasks, g_arena, "FiberTaskData");

	uint64 nextNumber = 1ull;
	for (uint64 i = 0ull; i < numTasks; ++i)
	{
		NumberSubset* subset = &pSubSets[i];

		subset->start = nextNumber;
		subset->end = nextNumber + numAdditionsPerTask - 1ull;
		if (subset->end > triangleNum) {
			subset->end = triangleNum;
		}

		pTasks[i] = { AddNumberSubset, subset };

		nextNumber = subset->end + 1;
	}

	core::TimeVal singleThreadElapse; 
	{
		core::StopWatch timer;

		for (uint64 i = 0ull; i < numTasks; ++i) {
			AddNumberSubset(nullptr, &pSubSets[i]);
		}

		// Add the results
		uint64 result = 0ull;
		for (uint64 i = 0; i < numTasks; ++i) {
			result += pSubSets[i].total;
		}

		singleThreadElapse = timer.GetTimeVal();

		EXPECT_EQ(expectedValue, result);
		EXPECT_EQ(numTasks, numJobsRan);
		X_LOG0("FiberScheduler", "Single threaded exec time: %f", singleThreadElapse.GetMilliSeconds());
	}

	nextNumber = 1ull;
	for (uint64 i = 0ull; i < numTasks; ++i)
	{
		NumberSubset *subset = &pSubSets[i];

		subset->start = nextNumber;
		subset->end = nextNumber + numAdditionsPerTask - 1ull;
		if (subset->end > triangleNum) {
			subset->end = triangleNum;
		}

		pTasks[i] = { AddNumberSubset, subset };
		nextNumber = subset->end + 1;
	}

	numJobsRan = 0;

	{
		Fiber::Scheduler scheduler;

		ASSERT_TRUE(scheduler.StartUp());

		core::TimeVal MultiElapsed;
		{
			core::Thread::SetName(core::Thread::GetCurrentID(), "MainThread");

			core::StopWatch timer;


			core::AtomicInt* pCounter = nullptr;
			scheduler.AddTasks(pTasks, numTasks, &pCounter);

			scheduler.WaitForCounterAndFree(pCounter, 0);

			MultiElapsed = timer.GetTimeVal();

			// Add the results
			uint64 result = 0ull;
			for (uint64 i = 0; i < numTasks; ++i) {
				result += pSubSets[i].total;
			}

			EXPECT_EQ(expectedValue, result);
			EXPECT_EQ(numTasks, numJobsRan);
		}

		X_DELETE_ARRAY(pTasks, g_arena);
		X_DELETE_ARRAY(pSubSets, g_arena);


		// work out percentage.
		// if it took 5 times less time it is 500%
		float32_t percentage = static_cast<float32_t>(singleThreadElapse.GetValue()) /
			static_cast<float32_t>(MultiElapsed.GetValue());

		percentage *= 100;

		// print the stats.
		X_LOG0("FiberScheduler", "Stats");
		X_LOG_BULLET;
		X_LOG0("FiberScheduler", "SingleThreaded: %g", singleThreadElapse.GetMilliSeconds());
		X_LOG0("FiberScheduler", "MultiThreaded: %g", MultiElapsed.GetMilliSeconds());
		X_LOG0("FiberScheduler", "Percentage: %g%% scaling: %g%%", percentage, percentage / scheduler.NumThreads());


		scheduler.ShutDown();
	}
}