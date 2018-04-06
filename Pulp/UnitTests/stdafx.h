
#pragma once

#include <EngineCommon.h>

#define GTEST_HAS_TR1_TUPLE 0
// if need tuple define this, fucking vs std/latest breaking shit.
// #define GTEST_USE_OWN_TR1_TUPLE 1
#include <gtest\gtest.h>

#include "UT_AssertChecker.h"

extern core::UtAssetCheckerHandler g_AssetChecker;

extern core::MemoryArenaBase* g_arena;
