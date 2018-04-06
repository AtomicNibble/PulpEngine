#include "stdafx.h"

#include <Containers\BitStream.h>
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

TEST(DISABLED_BitStreamTest, Genral)
{
    BitStream stream(g_arena);

    EXPECT_EQ(0, stream.capacity());
    EXPECT_EQ(0, stream.size());
    EXPECT_EQ(0, stream.freeSpace());

    stream.resize(1024);

    EXPECT_EQ(1024, stream.capacity());
    EXPECT_EQ(0, stream.size());
    EXPECT_EQ(1024, stream.freeSpace());

    for (int i = 0; i < 1024; i++) {
        bool is_true = (i % 4) == 0;
        stream.write(is_true);
    }

    EXPECT_EQ(1024, stream.capacity());
    EXPECT_EQ(1024, stream.size());
    EXPECT_EQ(0, stream.freeSpace());

    EXPECT_TRUE(stream.isEos());

    for (int i = 1023; i >= 0; i--) {
        bool is_true = (i % 4) == 0;
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

TEST(DISABLED_BitStreamTest, Small)
{
    BitStream stream(g_arena);

    EXPECT_EQ(0, stream.capacity());
    EXPECT_EQ(0, stream.size());
    EXPECT_EQ(0, stream.freeSpace());

    stream.resize(6);

    EXPECT_EQ(6, stream.capacity());
    EXPECT_EQ(0, stream.size());
    EXPECT_EQ(6, stream.freeSpace());

    for (int i = 0; i < 6; i++) {
        bool is_true = (i % 2) == 0;
        stream.write(is_true);
    }

    EXPECT_EQ(6, stream.capacity());
    EXPECT_EQ(6, stream.size());
    EXPECT_EQ(0, stream.freeSpace());

    EXPECT_TRUE(stream.isEos());

    for (int i = 5; i >= 0; i--) {
        bool is_true = (i % 2) == 0;
        EXPECT_EQ(is_true, stream.read());
    }

    stream.free();

    EXPECT_EQ(0, stream.capacity());
    EXPECT_EQ(0, stream.size());
    EXPECT_EQ(0, stream.freeSpace());
}

TEST(BitStreamTest, Move)
{
    const size_t bytes = (sizeof(uint8_t) * (100 + 64)) + (sizeof(BitStream) * 2) + (sizeof(size_t) * 3); // Linear header block.

    X_ALIGNED_SYMBOL(char buf[bytes], 8) = {};
    LinearAllocator allocator(buf, buf + bytes);
    LinearArea arena(&allocator, "MoveAllocator");

    Array<BitStream> list(&arena);
    list.setGranularity(2);
    list.reserve(2);

    list.push_back(BitStream(&arena, 100 * 8));
    list.push_back(BitStream(&arena, 64 * 8));
}

TEST(BitStreamTest, MoveAssign)
{
    const size_t bytes = (sizeof(uint8_t) * (16 + 64)) + (sizeof(size_t) * 2); // Linear header block.

    X_ALIGNED_SYMBOL(char buf[bytes], 8) = {};
    LinearAllocator allocator(buf, buf + bytes);
    LinearArea arena(&allocator, "MoveAllocator");

    // want the operator=(T&& oth) to be used.
    BitStream stream(&arena);

    stream.resize(16 * 8);
    stream = BitStream(&arena, 64 * 8);

    EXPECT_EQ(0, stream.size());
    EXPECT_EQ(64 * 8, stream.capacity());
}
