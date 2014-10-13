#include "stdafx.h"

#include <gtest\gtest.h>

X_USING_NAMESPACE;

using namespace core;


TEST(BitStreamTest, Genral)
{
	BitStream stream(g_arena);

	EXPECT_EQ(0, stream.capacity());
	EXPECT_EQ(0, stream.size());
	EXPECT_EQ(0, stream.freeSpace());

	stream.resize(1024);

	EXPECT_EQ(1024, stream.capacity());
	EXPECT_EQ(0, stream.size());
	EXPECT_EQ(1024, stream.freeSpace());

	for (int i = 0; i < 1024; i++)
	{
		bool is_true = (i % 4) == 0;
		stream.write(is_true);
	}

	EXPECT_EQ(1024, stream.capacity());
	EXPECT_EQ(1024, stream.size());
	EXPECT_EQ(0, stream.freeSpace());

	EXPECT_TRUE(stream.isEos());

	for (int i = 1023; i >= 0; i--)
	{
		bool is_true = (i % 4) == 0;
		EXPECT_EQ(is_true, stream.peek());
		EXPECT_EQ(is_true, stream.read());
	}

	// rekt
	EXPECT_EQ(1024, stream.capacity());
	EXPECT_EQ(0, stream.size());
	EXPECT_EQ(1024, stream.freeSpace());

	stream.free();

	EXPECT_EQ(0, stream.capacity());
	EXPECT_EQ(0, stream.size());
	EXPECT_EQ(0, stream.freeSpace());

}

TEST(BitStreamTest, Small)
{
	BitStream stream(g_arena);

	EXPECT_EQ(0, stream.capacity());
	EXPECT_EQ(0, stream.size());
	EXPECT_EQ(0, stream.freeSpace());

	stream.resize(6);

	EXPECT_EQ(6, stream.capacity());
	EXPECT_EQ(0, stream.size());
	EXPECT_EQ(6, stream.freeSpace());

	for (int i = 0; i < 6; i++)
	{
		bool is_true = (i % 2) == 0;
		stream.write(is_true);
	}

	EXPECT_EQ(6, stream.capacity());
	EXPECT_EQ(6, stream.size());
	EXPECT_EQ(0, stream.freeSpace());

	EXPECT_TRUE(stream.isEos());

	for (int i = 5; i >= 0; i--)
	{
		bool is_true = (i % 2) == 0;
		EXPECT_EQ(is_true, stream.peek());
		EXPECT_EQ(is_true, stream.read());
	}

	stream.free();

	EXPECT_EQ(0, stream.capacity());
	EXPECT_EQ(0, stream.size());
	EXPECT_EQ(0, stream.freeSpace());

}