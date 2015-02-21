#include "stdafx.h"

#include "gtest/gtest.h"
// slap goat, just don't tickle it's throat.
// or you will be going home in a boat.
// i wrote it's address on a note.
// be gone!


#include <Util\BitUtil.h>

X_USING_NAMESPACE;

using namespace core;

TEST(BitTest, IsSet) {

	for (int i = 0; i < 32; i++)
		EXPECT_FALSE(bitUtil::IsBitSet(0,i));

	for (int i = 0; i < 32; i++)
		EXPECT_TRUE(bitUtil::IsBitSet(-1, i));

	EXPECT_TRUE(bitUtil::IsBitSet(1, 0));

}

TEST(BitTest, Set) {

	uint32 val = 0;
	val = bitUtil::SetBit(val, 0);

	EXPECT_TRUE(bitUtil::IsBitSet(val, 0));

	// all others should not be set.
	for (int i = 1; i < 32; i++)
		EXPECT_FALSE(bitUtil::IsBitSet(val, i));

	val = 0;
	val = bitUtil::SetBit(val, 2);
	val = bitUtil::SetBit(val, 5);
	val = bitUtil::SetBit(val, 8);
	val = bitUtil::SetBit(val, 12);
	val = bitUtil::SetBit(val, 23);
	val = bitUtil::SetBit(val, 28);
	val = bitUtil::SetBit(val, 31);

	EXPECT_TRUE(bitUtil::IsBitSet(val, 2));
	EXPECT_TRUE(bitUtil::IsBitSet(val, 5));
	EXPECT_TRUE(bitUtil::IsBitSet(val, 8));
	EXPECT_TRUE(bitUtil::IsBitSet(val, 12));
	EXPECT_TRUE(bitUtil::IsBitSet(val, 23));
	EXPECT_TRUE(bitUtil::IsBitSet(val, 28));
	EXPECT_TRUE(bitUtil::IsBitSet(val, 31));
}

TEST(BitTest, NotSet) {

	uint32 val = 0;
	val = bitUtil::SetBit(val, 0);

	EXPECT_FALSE(bitUtil::IsBitNotSet(val, 0));

	// all others should not be set.
	for (int i = 1; i < 32; i++)
		EXPECT_TRUE(bitUtil::IsBitNotSet(val, i));

	val = 0;
	val = bitUtil::SetBit(val, 2);
	val = bitUtil::SetBit(val, 5);
	val = bitUtil::SetBit(val, 8);
	val = bitUtil::SetBit(val, 12);
	val = bitUtil::SetBit(val, 23);
	val = bitUtil::SetBit(val, 28);
	val = bitUtil::SetBit(val, 31);

	EXPECT_FALSE(bitUtil::IsBitNotSet(val, 2));
	EXPECT_FALSE(bitUtil::IsBitNotSet(val, 5));
	EXPECT_FALSE(bitUtil::IsBitNotSet(val, 8));
	EXPECT_FALSE(bitUtil::IsBitNotSet(val, 12));
	EXPECT_FALSE(bitUtil::IsBitNotSet(val, 23));
	EXPECT_FALSE(bitUtil::IsBitNotSet(val, 28));
	EXPECT_FALSE(bitUtil::IsBitNotSet(val, 31));
}

TEST(BitTest, Clear) {

	// clearning any bits should should still return 0.
	for (int i = 0; i < 32; i++)
		EXPECT_FALSE(bitUtil::ClearBit(0, i));

	uint32 val = ~0;
	for (int i = 0; i < 31; i++) {
		val = bitUtil::ClearBit(val, i);
		EXPECT_TRUE(val == (~0 << (i + 1)));
	}

}

TEST(BitTest, Scan) {

	uint32 val = 0;
	uint32 bit = 0;

	EXPECT_TRUE(bitUtil::NO_BIT_SET == bitUtil::ScanBits(val));

	val = 0x6464;
	EXPECT_TRUE(bitUtil::ScanBits(val) == 14);
	EXPECT_TRUE(bitUtil::ScanBitsForward(val) == 2);

	val = 0xf0;
	EXPECT_TRUE(bitUtil::ScanBits(val) == 7);
	EXPECT_TRUE(bitUtil::ScanBitsForward(val) == 4);

	val = 0xfab9;
	EXPECT_TRUE(bitUtil::ScanBits(val) == 15);
	EXPECT_TRUE(bitUtil::ScanBitsForward(val) == 0);
	
	val = 0x10000000;
	EXPECT_TRUE(bitUtil::ScanBits(val) == 28);
	EXPECT_TRUE(bitUtil::ScanBitsForward(val) == 28);

}

TEST(BitTest, Count) {


	EXPECT_TRUE(bitUtil::CountBits(0xf) == 4);
	EXPECT_TRUE(bitUtil::CountBits(0xff) == 8);
	EXPECT_TRUE(bitUtil::CountBits(0xfff) == 12);
	EXPECT_TRUE(bitUtil::CountBits(0xffff) == 16);

	EXPECT_TRUE(bitUtil::CountBits(0x2824122A) == 9);
	EXPECT_TRUE(bitUtil::CountBits(0xF1481669) == 14);

}

TEST(BitTest, PowerOfTwo) {

	EXPECT_TRUE(bitUtil::IsPowerOfTwo(2));
	EXPECT_TRUE(bitUtil::IsPowerOfTwo(4));
	EXPECT_TRUE(bitUtil::IsPowerOfTwo(8));
	EXPECT_TRUE(bitUtil::IsPowerOfTwo(16));
	EXPECT_TRUE(bitUtil::IsPowerOfTwo(32));
	EXPECT_TRUE(bitUtil::IsPowerOfTwo(32));
	EXPECT_TRUE(bitUtil::IsPowerOfTwo(64));
	EXPECT_TRUE(bitUtil::IsPowerOfTwo(128));
	EXPECT_TRUE(bitUtil::IsPowerOfTwo(256));

	EXPECT_TRUE(bitUtil::IsPowerOfTwo(256 * 64));
	EXPECT_TRUE(bitUtil::IsPowerOfTwo(256 * 128));
	EXPECT_TRUE(bitUtil::IsPowerOfTwo(256 * 256));
	EXPECT_TRUE(bitUtil::IsPowerOfTwo(256 * 512));

	EXPECT_FALSE(bitUtil::IsPowerOfTwo(3));
	EXPECT_FALSE(bitUtil::IsPowerOfTwo(7));
	EXPECT_FALSE(bitUtil::IsPowerOfTwo(9));
	EXPECT_FALSE(bitUtil::IsPowerOfTwo(17));
	EXPECT_FALSE(bitUtil::IsPowerOfTwo(38));
	EXPECT_FALSE(bitUtil::IsPowerOfTwo(94));

}

TEST(BitTest, RoundUp) {

	EXPECT_TRUE(bitUtil::RoundUpToMultiple(4, 8) == 8);
	EXPECT_TRUE(bitUtil::RoundUpToMultiple(0x29A7, 4) == 0x29A8);
	EXPECT_TRUE(bitUtil::RoundUpToMultiple(0x29A7, 8) == 0x29A8);

	EXPECT_TRUE(bitUtil::RoundUpToMultiple(200, 64) == 256);

}

TEST(BitTest, RoundDown) {

	EXPECT_TRUE(bitUtil::RoundDownToMultiple(4, 8) == 0);
	EXPECT_TRUE(bitUtil::RoundDownToMultiple(0x29A7, 4) == 0x29A4);
	EXPECT_TRUE(bitUtil::RoundDownToMultiple(0x29A7, 8) == 0x29A0);

	EXPECT_TRUE(bitUtil::RoundDownToMultiple(200, 64) == 192);

}

TEST(BitTest, NextPower2) {

	EXPECT_TRUE(core::bitUtil::NextPowerOfTwo(0) == 0);
	EXPECT_TRUE(core::bitUtil::NextPowerOfTwo(1) == 1);
	EXPECT_TRUE(core::bitUtil::NextPowerOfTwo(2) == 2);
	EXPECT_TRUE(core::bitUtil::NextPowerOfTwo(3) == 4);
	EXPECT_TRUE(core::bitUtil::NextPowerOfTwo(4) == 4);
	EXPECT_TRUE(core::bitUtil::NextPowerOfTwo(5) == 8);
}


TEST(BitTest, SignedBit)
{
	EXPECT_TRUE(bitUtil::isSignBitSet(-15252));
	EXPECT_TRUE(bitUtil::isSignBitSet(-626363));
	EXPECT_TRUE(bitUtil::isSignBitSet(-3));
	EXPECT_TRUE(bitUtil::isSignBitSet(-1));
	EXPECT_TRUE(bitUtil::isSignBitSet(4294967295));
	EXPECT_TRUE(bitUtil::isSignBitSet(2537551548));


	EXPECT_TRUE(bitUtil::isSignBitNotSet(62626));
	EXPECT_TRUE(bitUtil::isSignBitNotSet(525));
	EXPECT_TRUE(bitUtil::isSignBitNotSet(1));
	EXPECT_TRUE(bitUtil::isSignBitNotSet(0));
	EXPECT_TRUE(bitUtil::isSignBitNotSet(53636));
	EXPECT_TRUE(bitUtil::isSignBitNotSet(1246374));
}