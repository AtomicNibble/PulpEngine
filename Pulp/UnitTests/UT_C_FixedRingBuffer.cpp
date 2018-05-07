#include "stdafx.h"
#include "ComplexTypes.h"

#include <Containers\FixedRingBuffer.h>

X_USING_NAMESPACE;

using namespace core;


typedef ::testing::Types<short, int, float, testTypes::CustomType> MyTypes;
TYPED_TEST_CASE(FixedRingBufferTest, MyTypes);

template<typename T>
class FixedRingBufferTest : public ::testing::Test
{
public:
};


TYPED_TEST(FixedRingBufferTest, Types)
{
    testTypes::resetConConters();

    FixedRingBuffer<TypeParam, 3> ring;

    EXPECT_EQ(0, ring.size());
    ASSERT_EQ(3, ring.capacity());
    ASSERT_EQ(3, ring.freeSpace());

    ring.append(16);
    EXPECT_EQ(1, ring.size());
    ASSERT_EQ(3, ring.capacity());
    ASSERT_EQ(2, ring.freeSpace());

    ring.append(32);
    EXPECT_EQ(2, ring.size());
    ASSERT_EQ(3, ring.capacity());
    ASSERT_EQ(1, ring.freeSpace());

    ring.append(48);
    EXPECT_EQ(3, ring.size());
    ASSERT_EQ(3, ring.capacity());
    ASSERT_EQ(0, ring.freeSpace());

    EXPECT_EQ(16, ring[0]);
    EXPECT_EQ(32, ring[1]);
    EXPECT_EQ(48, ring[2]);

    ring.append(96);
    EXPECT_EQ(3, ring.size());
    ASSERT_EQ(3, ring.capacity());
    ASSERT_EQ(0, ring.freeSpace());

    EXPECT_EQ(32, ring[0]);
    EXPECT_EQ(48, ring[1]);
    EXPECT_EQ(96, ring[2]);
}


TYPED_TEST(FixedRingBufferTest, Index)
{
    testTypes::resetConConters();

    FixedRingBuffer<TypeParam, 3> ring;

    ring.append(16);
    ring.append(32);
    ring.append(48);
    EXPECT_EQ(3, ring.size());
    ASSERT_EQ(3, ring.capacity());
    ASSERT_EQ(0, ring.freeSpace());

    EXPECT_EQ(16, ring[0]);
    EXPECT_EQ(32, ring[1]);
    EXPECT_EQ(48, ring[2]);

    ring.append(96);
    EXPECT_EQ(96, ring[2]);

    ring.append(112);
    EXPECT_EQ(112, ring[2]);

    ring.append(128);
    EXPECT_EQ(128, ring[2]);
}


TEST(FixedRingBufferTest, ComplexConstruction)
{
    testTypes::resetConConters();

    FixedRingBuffer<testTypes::CustomType, 3> ring;

    EXPECT_EQ(0, ring.size());
    ASSERT_EQ(3, ring.capacity());

    ring.append(16);
    ring.append(32);
    ring.append(48);

    EXPECT_EQ(0, testTypes::MOVE_COUNT);
    EXPECT_EQ(3 + 3, testTypes::CONSRUCTION_COUNT); // 3 rvalues
    EXPECT_EQ(3, testTypes::DECONSRUCTION_COUNT);

    ring.append(96);
    EXPECT_EQ(3, ring.size());
    ASSERT_EQ(3, ring.capacity());

    // 1 rvalue + assignment
    EXPECT_EQ(0, testTypes::MOVE_COUNT);
    EXPECT_EQ(6 + 1, testTypes::CONSRUCTION_COUNT);
    EXPECT_EQ(3 + 1, testTypes::DECONSRUCTION_COUNT);
}

TEST(FixedRingBufferTest, ComplexEmplace)
{
    testTypes::resetConConters();

    FixedRingBuffer<testTypes::CustomType, 3> ring;

    EXPECT_EQ(0, ring.size());
    ASSERT_EQ(3, ring.capacity());

    ring.emplace_back(16);
    ring.emplace_back(32);
    ring.emplace_back(48);

    EXPECT_EQ(0, testTypes::MOVE_COUNT);
    EXPECT_EQ(3, testTypes::CONSRUCTION_COUNT); // 3 rvalues
    EXPECT_EQ(0, testTypes::DECONSRUCTION_COUNT);

    ring.emplace_back(96);
    EXPECT_EQ(3, ring.size());
    ASSERT_EQ(3, ring.capacity());

    // 1 rvalue + assignment
    EXPECT_EQ(0, testTypes::MOVE_COUNT);
    EXPECT_EQ(4, testTypes::CONSRUCTION_COUNT);
    EXPECT_EQ(1, testTypes::DECONSRUCTION_COUNT);
}

