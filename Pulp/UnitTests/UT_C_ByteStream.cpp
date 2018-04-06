#include "stdafx.h"

#include <Containers\ByteStream.h>
#include <Containers\Array.h> // for move test.

#include <Memory\BoundsCheckingPolicies\NoBoundsChecking.h>
#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>
#include <Memory\AllocationPolicies\LinearAllocator.h>
#include <Memory\MemoryTaggingPolicies\NoMemoryTagging.h>

X_USING_NAMESPACE;

using namespace core;

typedef core::MemoryArena<
    core::LinearAllocator,
    core::SingleThreadPolicy,
    core::NoBoundsChecking,
    core::NoMemoryTracking,
    core::NoMemoryTagging>
    LinearArea;

TEST(ByteStreamTest, Genral)
{
    ByteStream stream(g_arena);

    EXPECT_EQ(0, stream.capacity());
    EXPECT_EQ(0, stream.size());
    EXPECT_EQ(0, stream.freeSpace());

    stream.reserve(1024);

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

    for (int i = 0; i < 250; i++) {
        stream.write(i);
    }

    EXPECT_EQ(1024, stream.capacity());
    EXPECT_EQ(1024, stream.size());
    EXPECT_EQ(0, stream.freeSpace());

    EXPECT_TRUE(stream.isEos());

    EXPECT_EQ(1, stream.read<uint8>());
    EXPECT_FLOAT_EQ(fval, stream.read<float>());
    EXPECT_EQ(2, stream.read<uint8>());
    EXPECT_EQ(Col_Aliceblue, stream.read<Colorf>());
    EXPECT_EQ(3, stream.read<uint8>());
    EXPECT_EQ(4, stream.read<uint8>());

    EXPECT_EQ(1000, stream.size());

    for (int i = 0; i < 250; i++) {
        EXPECT_EQ(i, stream.peek<int>());
        EXPECT_EQ(i, stream.read<int>());
    }

    // rekt
    EXPECT_EQ(1024, stream.capacity());
    EXPECT_EQ(0, stream.size());
    EXPECT_EQ(0, stream.freeSpace());

    stream.reset();

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

    stream.reserve(1024);

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

    for (int i = 0; i < 250; i++) {
        stream.write(i);
    }

    EXPECT_EQ(1024, stream.capacity());
    EXPECT_EQ(1024, stream.size());
    EXPECT_EQ(0, stream.freeSpace());

    EXPECT_TRUE(stream.isEos());

    // grow a bit.
    stream.reserve(2048);

    EXPECT_EQ(2048, stream.capacity());
    EXPECT_EQ(1024, stream.size());
    EXPECT_EQ(1024, stream.freeSpace());

    EXPECT_FALSE(stream.isEos());

    EXPECT_EQ(1, stream.read<uint8>());
    EXPECT_FLOAT_EQ(fval, stream.read<float>());
    EXPECT_EQ(2, stream.read<uint8>());
    EXPECT_EQ(Col_Aliceblue, stream.read<Colorf>());
    EXPECT_EQ(3, stream.read<uint8>());
    EXPECT_EQ(4, stream.read<uint8>());

    EXPECT_EQ(1000, stream.size());

    // check the first items are still correct.
    for (int i = 0; i < 250; i++) {
        EXPECT_EQ(i, stream.peek<int>());
        EXPECT_EQ(i, stream.read<int>());
    }

    // rekt
    EXPECT_EQ(2048, stream.capacity());
    EXPECT_EQ(0, stream.size());
    EXPECT_EQ(1024, stream.freeSpace());

    stream.reset();

    EXPECT_EQ(2048, stream.capacity());
    EXPECT_EQ(0, stream.size());
    EXPECT_EQ(2048, stream.freeSpace());

    stream.free();

    EXPECT_EQ(0, stream.capacity());
    EXPECT_EQ(0, stream.size());
    EXPECT_EQ(0, stream.freeSpace());
}

TEST(ByteStreamTest, Move)
{
    const size_t bytes = (sizeof(uint8_t) * (100 + 64)) + (sizeof(ByteStream) * 2) + (sizeof(size_t) * 3); // Linear header block.

    X_ALIGNED_SYMBOL(char buf[bytes], 8) = {};
    LinearAllocator allocator(buf, buf + bytes);
    LinearArea arena(&allocator, "MoveAllocator");

    Array<ByteStream> list(&arena);
    list.setGranularity(2);
    list.reserve(2);

    ByteStream strm(&arena);
    strm.reserve(100);
    list.push_back(std::move(strm));

    ByteStream strm1(&arena);
    strm1.reserve(64);

    list.push_back(std::move(strm1));
}

TEST(ByteStreamTest, MoveAssign)
{
    const size_t bytes = (sizeof(uint8_t) * (16 + 64)) + (sizeof(size_t) * 2); // Linear header block.

    X_ALIGNED_SYMBOL(char buf[bytes], 8) = {};
    LinearAllocator allocator(buf, buf + bytes);
    LinearArea arena(&allocator, "MoveAllocator");

    // want the operator=(T&& oth) to be used.
    ByteStream stream(&arena);
    ByteStream stream2(&arena);

    stream2.reserve(64);

    stream.reserve(16);
    stream = std::move(stream2);

    EXPECT_EQ(0, stream.size());
    EXPECT_EQ(64, stream.capacity());

    EXPECT_EQ(0, stream2.size());
    EXPECT_EQ(0, stream2.capacity());
}

TEST(ByteStreamTest, EmptyCopyCon)
{
    // check that constructing a stream from a empty stream is valid.

    ByteStream empty(g_arena);
    ByteStream strm(empty);

    ASSERT_EQ(0, strm.size());
    ASSERT_EQ(0, strm.capacity());
    ASSERT_EQ(0, strm.freeSpace());

    ASSERT_TRUE(strm.begin() == nullptr);

    strm.free();
    strm.reserve(16);

    ASSERT_EQ(0, strm.size());
    ASSERT_EQ(16, strm.capacity());
    ASSERT_EQ(16, strm.freeSpace());

    strm.write<uint8_t>(0xff);

    ASSERT_EQ(1, strm.size());
    ASSERT_EQ(16, strm.capacity());
    ASSERT_EQ(15, strm.freeSpace());
}