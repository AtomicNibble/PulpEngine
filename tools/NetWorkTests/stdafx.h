

#include <EngineCommon.h>

#include <INetwork.h>
#include <NetMsgIds.h>

#include "Constants.h"

#include <String\HumanSize.h>
#include <Time\StopWatch.h>

#include <Containers\Array.h>
#include <Containers\FixedBitStream.h>

#define GTEST_HAS_TR1_TUPLE 0
#include <gtest\gtest.h>

extern core::MemoryArenaBase* g_arena;

// Google Test
#if X_DEBUG == 1
X_LINK_LIB("gtestd")
#else
X_LINK_LIB("gtest")
#endif