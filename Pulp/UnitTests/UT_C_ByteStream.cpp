#include "stdafx.h"

#include <gtest\gtest.h>

X_USING_NAMESPACE;

using namespace core;


TEST(ByteStreamTest, Genral)
{
	ByteStream stream(g_arena);

	EXPECT_EQ(0, stream.capacity());
	EXPECT_EQ(0, stream.size());
	EXPECT_EQ(0, stream.freeSpace());

	stream.resize(1024);

	EXPECT_EQ(1024, stream.capacity());
	EXPECT_EQ(0, stream.size());
	EXPECT_EQ(1024, stream.freeSpace());

	float fval = 0.56789f;

	// write 24 bytes
	stream.write<uint8>(1);
	stream.write(fval);
	stream.write<uint8>(2);
	stream.write(Col_Aliceblue);
	stream.write<uint8>(3);
	stream.write<uint8>(4);

	EXPECT_EQ(24, stream.size());

	for (int i = 0; i < 250; i++)
	{
		stream.write(i);
	}

	EXPECT_EQ(1024, stream.capacity());
	EXPECT_EQ(1024, stream.size());
	EXPECT_EQ(0, stream.freeSpace());

	EXPECT_TRUE(stream.isEos());

	for (int i = 249; i >= 0; i--)
	{
		EXPECT_EQ(i, stream.peek<int>());
		EXPECT_EQ(i, stream.read<int>());
	}

	EXPECT_EQ(24, stream.size());


	EXPECT_EQ(4, stream.read<uint8>());
	EXPECT_EQ(3, stream.read<uint8>());
	EXPECT_EQ(Col_Aliceblue, stream.read<Colorf>());
	EXPECT_EQ(2, stream.read<uint8>());
	EXPECT_FLOAT_EQ(fval, stream.read<float>());
	EXPECT_EQ(1, stream.read<uint8>());

	// rekt
	EXPECT_EQ(1024, stream.capacity());
	EXPECT_EQ(0, stream.size());
	EXPECT_EQ(1024, stream.freeSpace());

	stream.free();

	EXPECT_EQ(0, stream.capacity());
	EXPECT_EQ(0, stream.size());
	EXPECT_EQ(0, stream.freeSpace());
}

// test that growing keeps old items.
TEST(ByteStreamTest, Grow)
{
	ByteStream stream(g_arena);

	EXPECT_EQ(0, stream.capacity());
	EXPECT_EQ(0, stream.size());
	EXPECT_EQ(0, stream.freeSpace());

	stream.resize(1024);

	EXPECT_EQ(1024, stream.capacity());
	EXPECT_EQ(0, stream.size());
	EXPECT_EQ(1024, stream.freeSpace());

	float fval = 0.56789f;

	// write 24 bytes
	stream.write<uint8>(1);
	stream.write(fval);
	stream.write<uint8>(2);
	stream.write(Col_Aliceblue);
	stream.write<uint8>(3);
	stream.write<uint8>(4);

	EXPECT_EQ(24, stream.size());

	for (int i = 0; i < 250; i++)
	{
		stream.write(i);
	}

	EXPECT_EQ(1024, stream.capacity());
	EXPECT_EQ(1024, stream.size());
	EXPECT_EQ(0, stream.freeSpace());

	EXPECT_TRUE(stream.isEos());

	// grow a bit.
	stream.resize(2048);

	EXPECT_EQ(2048, stream.capacity());
	EXPECT_EQ(1024, stream.size());
	EXPECT_EQ(1024, stream.freeSpace());

	EXPECT_FALSE(stream.isEos());

	// check the first items are still correct.
	for (int i = 249; i >= 0; i--)
	{
		EXPECT_EQ(i, stream.peek<int>());
		EXPECT_EQ(i, stream.read<int>());
	}

	EXPECT_EQ(24, stream.size());

	EXPECT_EQ(4, stream.read<uint8>());
	EXPECT_EQ(3, stream.read<uint8>());
	EXPECT_EQ(Col_Aliceblue, stream.read<Colorf>());
	EXPECT_EQ(2, stream.read<uint8>());
	EXPECT_FLOAT_EQ(fval, stream.read<float>());
	EXPECT_EQ(1, stream.read<uint8>());

	// rekt
	EXPECT_EQ(2048, stream.capacity());
	EXPECT_EQ(0, stream.size());
	EXPECT_EQ(2048, stream.freeSpace());
}