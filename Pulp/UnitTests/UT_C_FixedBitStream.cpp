#include "stdafx.h"

#include <Containers\FixedBitStream.h>

X_USING_NAMESPACE;

typedef core::FixedBitStream<core::FixedBitStreamNoneOwningPolicy> FixedBitStream;

TEST(FixBitStreamBuf, ConstructEmpty)
{
    core::FixedBitStreamNoneOwning emptyBS;

    EXPECT_TRUE(emptyBS.isEos());
    EXPECT_TRUE(emptyBS.isStartOfStream());
    EXPECT_EQ(0, emptyBS.size());
    EXPECT_EQ(0, emptyBS.sizeInBytes());
    EXPECT_EQ(0, emptyBS.capacity());

    EXPECT_EQ(emptyBS.begin(), emptyBS.end());
}

TEST(FixBitStreamBuf, SimpleBits)
{
    uint8_t buf[0x200];
    FixedBitStream stream(buf, buf + sizeof(buf), false);

    EXPECT_EQ(core::bitUtil::bytesToBits(sizeof(buf)), stream.capacity());
    EXPECT_EQ(0, stream.size());
    EXPECT_EQ(stream.capacity(), stream.freeSpace());

    size_t totalBits = 0;

    // simple test.
    // just write a load of 3 bit 2's
    // resulting in 010010010010010
    for (size_t i = 0; i < 0x100; i++) {
        uint8_t val = 2;
        uint8_t numBits = 3;

        stream.writeBits(&val, numBits);

        totalBits += numBits;
        EXPECT_EQ(totalBits, stream.size());
    }

    // read the out.
    size_t totalBitsLeft = stream.size();
    for (size_t i = 0; i < 0x100; i++) {
        uint8_t expectedVal = 2;
        uint8_t numBits = 3;

        uint8_t val = 0;
        stream.readBits(&val, numBits);

        totalBitsLeft -= numBits;

        EXPECT_EQ((int32_t)expectedVal, (int32_t)val);
        EXPECT_EQ(totalBitsLeft, stream.size());
    }
}

TEST(FixBitStreamBuf, SimpleBits2)
{
    uint8_t buf[0x200];
    FixedBitStream stream(buf, buf + sizeof(buf), false);

    EXPECT_EQ(core::bitUtil::bytesToBits(sizeof(buf)), stream.capacity());
    EXPECT_EQ(0, stream.size());
    EXPECT_EQ(stream.capacity(), stream.freeSpace());

    size_t totalBits = 0;

    // simple test.
    // just write a load of 3 bit 2's
    // resulting in 010010010010010
    for (size_t i = 0; i < 0x100; i++) {
        uint8_t val = 13;
        uint8_t numBits = 5;

        stream.writeBits(&val, numBits);

        totalBits += numBits;
        EXPECT_EQ(totalBits, stream.size());
    }

    // read the out.
    size_t totalBitsLeft = stream.size();
    for (size_t i = 0; i < 0x100; i++) {
        uint8_t expectedVal = 13;
        uint8_t numBits = 5;

        uint8_t val = 0;
        stream.readBits(&val, numBits);

        totalBitsLeft -= numBits;

        EXPECT_EQ((int32_t)expectedVal, (int32_t)val);
        EXPECT_EQ(totalBitsLeft, stream.size());
    }
}

TEST(FixBitStreamBuf, SimpleBits3)
{
    uint8_t buf[0x200];
    FixedBitStream stream(buf, buf + sizeof(buf), false);

    EXPECT_EQ(core::bitUtil::bytesToBits(sizeof(buf)), stream.capacity());
    EXPECT_EQ(0, stream.size());
    EXPECT_EQ(stream.capacity(), stream.freeSpace());

    size_t totalBits = 0;

    // simple test.
    // just write a load of 3 bit 2's
    // resulting in 010010010010010
    for (size_t i = 0; i < 0x100; i++) {
        uint16_t val = 365;
        uint8_t numBits = 9;

        stream.writeBits((uint8_t*)&val, numBits);

        totalBits += numBits;
        EXPECT_EQ(totalBits, stream.size());
    }

    // read the out.
    size_t totalBitsLeft = stream.size();
    for (size_t i = 0; i < 0x100; i++) {
        uint16_t expectedVal = 365;
        uint8_t numBits = 9;

        uint16_t val = 0;
        stream.readBits((uint8_t*)&val, numBits);

        totalBitsLeft -= numBits;

        EXPECT_EQ((int32_t)expectedVal, (int32_t)val);
        EXPECT_EQ(totalBitsLeft, stream.size());
    }
}

TEST(FixBitStreamBuf, VariaingBitSizes)
{
    uint8_t buf[0x200];
    FixedBitStream stream(buf, buf + sizeof(buf), false);

    EXPECT_EQ(core::bitUtil::bytesToBits(sizeof(buf)), stream.capacity());
    EXPECT_EQ(0, stream.size());
    EXPECT_EQ(stream.capacity(), stream.freeSpace());

    // write some bits, or yer baby no bytes here!
    size_t totalBits = 0;

    for (size_t i = 0; i < 100; i++) {
        uint8_t val = static_cast<uint8_t>(i);
        uint8_t numBits = core::Max<uint8_t>(1u, core::bitUtil::bitsNeededForValue(val));

        stream.writeBits(&val, numBits);

        totalBits += numBits;
        EXPECT_EQ(totalBits, stream.size());
    }

    // read the out.
    size_t totalBitsLeft = stream.size();
    for (size_t i = 0; i < 100; i++) {
        uint8_t expectedVal = static_cast<uint8_t>(i);
        uint8_t numBits = core::Max<uint8_t>(1u, core::bitUtil::bitsNeededForValue(expectedVal));

        uint8_t val = 0;
        stream.readBits(&val, numBits);

        totalBitsLeft -= numBits;

        EXPECT_EQ((int32_t)expectedVal, (int32_t)val);
        EXPECT_EQ(totalBitsLeft, stream.size());
    }
}


TEST(FixBitStreamBuf, StackString)
{
    uint8_t buf[0x200];
    FixedBitStream stream(buf, buf + sizeof(buf), false);

    EXPECT_EQ(core::bitUtil::bytesToBits(sizeof(buf)), stream.capacity());
    EXPECT_EQ(0, stream.size());
    EXPECT_EQ(stream.capacity(), stream.freeSpace());

    const core::StackString<128, char> narrowStr("Do you like green eggs and ham?");
    const core::StackString<128, wchar_t> wideStr("Fuck you and your green food!");

    stream.write(narrowStr);
    stream.write(wideStr);
    stream.write(narrowStr);
    stream.write(wideStr);

    core::StackString<128, char> narrowOut0, narrowOut1;
    core::StackString<128, wchar_t> wideOut0, wideOut1;

    stream.read(narrowOut0);
    stream.read(wideOut0);
    stream.read(narrowOut1);
    stream.read(wideOut1);

    EXPECT_TRUE(stream.isEos());

    EXPECT_EQ(narrowStr, narrowOut0);
    EXPECT_EQ(narrowStr, narrowOut1);
    EXPECT_EQ(wideStr, wideOut0);
    EXPECT_EQ(wideStr, wideOut1);

}