#include "stdafx.h"

#include <Containers\FixedByteStreamRing.h>

X_USING_NAMESPACE;

typedef core::FixedByteStreamRing<core::FixedByteStreamRingNoneOwningPolicy> FixedByteStreamRing;

TEST(FixByteStreamRing, SimpleBytes)
{
    uint8_t buf[0x200];
    FixedByteStreamRing stream(buf, buf + sizeof(buf), false);

    EXPECT_EQ(sizeof(buf), stream.capacity());
    EXPECT_EQ(0, stream.size());
    EXPECT_EQ(stream.capacity(), stream.freeSpace());

    for (int32_t i = 0; i < 0x100; i++) {
        stream.write<uint8_t>(i & 0xFF);
    }

    EXPECT_EQ(sizeof(buf), stream.capacity());
    EXPECT_EQ(0x100, stream.size());
    EXPECT_EQ(0x100, stream.freeSpace());

    for (int32_t i = 0; i < 0x100; i++) {
        uint8_t val = 0;
        stream.read(val);

        EXPECT_EQ(i, (int32_t)val);
    }

    EXPECT_EQ(sizeof(buf), stream.capacity());
    EXPECT_EQ(0, stream.size());
    EXPECT_EQ(stream.capacity(), stream.freeSpace()); // since it's ring capacity should now be usable.

    // just resets the ring.
    stream.reset();

    EXPECT_EQ(sizeof(buf), stream.capacity());
    EXPECT_EQ(0, stream.size());
    EXPECT_EQ(stream.capacity(), stream.freeSpace());
}

TEST(FixByteStreamRing, Fill)
{
    // Make sure if we fill it completelty size is correct.
    uint8_t buf[0x100];
    FixedByteStreamRing stream(buf, buf + sizeof(buf), false);

    EXPECT_EQ(sizeof(buf), stream.capacity());
    EXPECT_EQ(0, stream.size());
    EXPECT_EQ(stream.capacity(), stream.freeSpace());

    for (int32_t i = 0; i < 0x100; i++) {
        stream.write<uint8_t>(i & 0xFF);
    }

    EXPECT_EQ(sizeof(buf), stream.capacity());
    EXPECT_EQ(0x100, stream.size());
    EXPECT_EQ(0, stream.freeSpace());
}

TEST(FixByteStreamRing, FillWrapped)
{
    // fill it but wrapping.
    uint8_t buf[0x100];
    FixedByteStreamRing stream(buf, buf + sizeof(buf), false);

    EXPECT_EQ(sizeof(buf), stream.capacity());
    EXPECT_EQ(0, stream.size());
    EXPECT_EQ(stream.capacity(), stream.freeSpace());

    for (int32_t i = 0; i < 0x100; i++) {
        stream.write<uint8_t>(i & 0xFF);
    }

    for (int32_t i = 0; i < 0x80; i++) {
        uint8_t val = 0;
        stream.read(val);
        EXPECT_EQ(i, (int32_t)val);
    }

    EXPECT_EQ(sizeof(buf), stream.capacity());
    EXPECT_EQ(0x80, stream.size());
    EXPECT_EQ(0x80, stream.freeSpace());

    for (int32_t i = 0; i < 0x80; i++) {
        stream.write<uint8_t>(i & 0xFF);
    }

    EXPECT_EQ(sizeof(buf), stream.capacity());
    EXPECT_EQ(0x100, stream.size());
    EXPECT_EQ(0, stream.freeSpace());
}