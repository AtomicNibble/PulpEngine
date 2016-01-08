#include "stdafx.h"



#include <Random\XorShift.h>
#include <Random\MultiplyWithCarry.h>


X_USING_NAMESPACE;

using namespace core;

TEST(Random, Shift) {

	for (uint i = 0; i<1000; ++i)
	{
		const uint32_t random = random::XorShift(10u, 400u);
		EXPECT_TRUE(random >= 10 && random < 400);
	}

	for (uint i = 0; i<1000; ++i)
	{
		const uint32_t random = random::XorShift(15u, 16u);
		EXPECT_TRUE(random == 15);
	}

	// float
	for (uint i = 0; i<1000; ++i)
	{
		const float32_t random = random::XorShift(-500.f, 1000.f);
		EXPECT_TRUE(random >= -500.f && random < 1000.f);
	}


}

TEST(Random, Multiply) {

	for (uint i = 0; i<1000; ++i)
	{
		const uint32_t random = random::MultiplyWithCarry(10u, 400u);
		EXPECT_TRUE(random >= 10 && random < 400);
	}

	for (uint i = 0; i<1000; ++i)
	{
		const uint32_t random = random::MultiplyWithCarry(15u, 16u);
		EXPECT_TRUE(random == 15);
	}

	// float
	for (uint i = 0; i<1000; ++i)
	{
		const float32_t random = random::MultiplyWithCarry(-500.f, 1000.f);
		EXPECT_TRUE(random >= -500.f && random < 1000.f);
	}
}