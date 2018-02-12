#include "stdafx.h"



#include <Random\XorShift.h>
#include <Random\MultiplyWithCarry.h>


X_USING_NAMESPACE;

using namespace core;

TEST(Random, Shift) {

	core::random::XorShift shift;

	for (uint i = 0; i<1000; ++i)
	{
		const uint32_t random = shift.randRange(10u, 400u);
		EXPECT_TRUE(random >= 10 && random <= 400);
	}

	for (uint i = 0; i<1000; ++i)
	{
		const uint32_t random = shift.randRange(15u, 16u);
		EXPECT_TRUE(random == 15 || random == 16);
	}

	for (uint i = 0; i<1000; ++i)
	{
		const size_t random = shift.randIndex(100_sz);
		EXPECT_TRUE(random < 100_sz);
	}

	// float
	for (uint i = 0; i<1000; ++i)
	{
		const float32_t random = shift.randRange(-500.f, 1000.f);
		EXPECT_TRUE(random >= -500.f && random <= 1000.f);
	}

	for (uint i = 0; i<1000; ++i)
	{
		const float32_t random = shift.randRange(1000.f);
		EXPECT_TRUE(random >= 0.f && random <= 1000.f);
	}
}

TEST(Random, Shift128) {

	core::random::XorShift128 shift;

	for (uint i = 0; i<1000; ++i)
	{
		const uint64_t random = shift.randRange(10ull, 400ull);
		EXPECT_TRUE(random >= 10 && random <= 400);
	}

	for (uint i = 0; i<1000; ++i)
	{
		const uint64_t random = shift.randRange(15ull, 16ull);
		EXPECT_TRUE(random == 15 || random == 16);
	}

	for (uint i = 0; i<1000; ++i)
	{
		const size_t random = shift.randIndex(100_sz);
		EXPECT_TRUE(random < 100_sz);
	}

	// float
	for (uint i = 0; i<1000; ++i)
	{
		const float32_t random = shift.randRange(-500.f, 1000.f);
		EXPECT_TRUE(random >= -500.f && random <= 1000.f);
	}

	for (uint i = 0; i<1000; ++i)
	{
		const float32_t random = shift.randRange(1000.f);
		EXPECT_TRUE(random >= 0.f && random <= 1000.f);
	}
}

TEST(Random, Multiply) {

	core::random::MultiplyWithCarry mwc;

	for (uint i = 0; i<1000; ++i)
	{
		const uint32_t random = mwc.randRange(10u, 400u);
		EXPECT_TRUE(random >= 10 && random <= 400);
	}

	for (uint i = 0; i<1000; ++i)
	{
		const uint32_t random = mwc.randRange(15u, 16u);
		EXPECT_TRUE(random == 15 || random == 16);
	}

	for (uint i = 0; i<1000; ++i)
	{
		const size_t random = mwc.randIndex(100_sz);
		EXPECT_TRUE(random < 100_sz);
	}

	// float
	for (uint i = 0; i<1000; ++i)
	{
		const float32_t random = mwc.randRange(-500.f, 1000.f);
		EXPECT_TRUE(random >= -500.f && random < 1000.f);
	}

	for (uint i = 0; i<1000; ++i)
	{
		const float32_t random = mwc.randRange(1000.f);
		EXPECT_TRUE(random >= 0.f && random <= 1000.f);
	}
}

