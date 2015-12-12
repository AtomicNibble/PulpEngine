#include "stdafx.h"

#include "Threading\FiberScheduler.h"

#include "gtest/gtest.h"


X_USING_NAMESPACE;

using namespace core;

namespace
{
	X_PRAGMA(optimize("", off))

	struct NumberSubset
	{
		uint64 start;
		uint64 end;
		uint64 total;
	};

	void AddNumberSubset(Fiber::Scheduler* pScheduler, void* pArg)
	{
		NumberSubset* pSubSet = reinterpret_cast<NumberSubset*>(pArg);

		pSubSet->total = 0;

		while (pSubSet->start != pSubSet->end) {
			pSubSet->total += pSubSet->start;
			++pSubSet->start;
		}

		pSubSet->total += pSubSet->end;
		}

	X_PRAGMA(optimize("", on))
}



TEST(Threading, FiberScheduler)
{
	Fiber::Scheduler scheduler;

	ASSERT_TRUE(scheduler.StartUp());

	// Define the constants to test
	const uint64 triangleNum = 47593243ull;
	const uint64 numAdditionsPerTask = 10000ull;
	const uint64 numTasks = (triangleNum + numAdditionsPerTask - 1ull) / numAdditionsPerTask;

	Fiber::Task* pTasks = X_NEW_ARRAY(Fiber::Task, numTasks, g_arena, "FiberTask");
	NumberSubset* pSubSets = X_NEW_ARRAY(NumberSubset, numTasks, g_arena, "FiberTaskData");

	uint64 nextNumber = 1ull;

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

	core::Thread::SetName(core::Thread::GetCurrentID(), "MainThread");

	core::AtomicInt* pCounter = nullptr;
	scheduler.AddTasks(pTasks, numTasks, &pCounter);

	// these can be deleted now.
	X_DELETE_ARRAY(pTasks, g_arena);


	scheduler.WaitForCounterAndFree(pCounter, 0);

	// Add the results
	uint64 result = 0ull;
	for (uint64 i = 0; i < numTasks; ++i) {
		result += pSubSets[i].total;
	}

	const uint64 expectedValue = triangleNum * (triangleNum + 1ull) / 2ull;
	EXPECT_EQ(expectedValue, result);


	X_DELETE_ARRAY(pSubSets, g_arena);

	scheduler.ShutDown();
}