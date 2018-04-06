#include "stdafx.h"

#include <Containers\FixedByteStream.h>

X_USING_NAMESPACE;

typedef core::FixedByteStream<core::FixedByteStreamNoneOwningPolicy> FixedByteStream;

TEST(FixByteStreamBuf, SimpleBytes)
{
    uint8_t buf[0x200];
    FixedByteStream stream(buf, buf + sizeof(buf), false);

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
    EXPECT_EQ(0x100, stream.freeSpace());

    stream.reset();

    EXPECT_EQ(sizeof(buf), stream.capacity());
    EXPECT_EQ(0, stream.size());
    EXPECT_EQ(stream.capacity(), stream.freeSpace());
}