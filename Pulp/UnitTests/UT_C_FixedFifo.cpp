#include "stdafx.h"

#include <Containers\FixedFifo.h>

X_USING_NAMESPACE;

using namespace core;

namespace
{
    struct CustomType2
    {
        static int CONSTRUCTION_COUNT;
        static int DECONSTRUCTION_COUNT;

        CustomType2() :
            val_(0)
        {
            CONSTRUCTION_COUNT++;
        }
        CustomType2(int val) :
            val_(val)
        {
            CONSTRUCTION_COUNT++;
        }
        CustomType2(const CustomType2& oth) :
            val_(oth.val_)
        {
            CONSTRUCTION_COUNT++;
        }
        ~CustomType2()
        {
            DECONSTRUCTION_COUNT++;
        }

        CustomType2& operator+=(const CustomType2& oth)
        {
            val_ += oth.val_;
            return *this;
        }

        operator int() const
        {
            return val_;
        }

    private:
        int val_;
    };

    int CustomType2::CONSTRUCTION_COUNT = 0;
    int CustomType2::DECONSTRUCTION_COUNT = 0;
} // namespace

typedef ::testing::Types<short, int, float, CustomType2> MyTypes;
TYPED_TEST_CASE(FixedFifoTest, MyTypes);

template<typename T>
class FixedFifoTest : public ::testing::Test
{
public:
};

TYPED_TEST(FixedFifoTest, Types)
{
    {
        FixedFifo<TypeParam, 3> fifo;

        EXPECT_EQ(0, fifo.size());
        ASSERT_EQ(3, fifo.capacity());

        // Push

        fifo.push(16);
        EXPECT_EQ(1, fifo.size());
        ASSERT_EQ(3, fifo.capacity());

        fifo.push(32);
        EXPECT_EQ(2, fifo.size());
        ASSERT_EQ(3, fifo.capacity());

        fifo.push(48);
        EXPECT_EQ(3, fifo.size());
        ASSERT_EQ(3, fifo.capacity());

        // Pop

        EXPECT_EQ(16, fifo.peek());
        fifo.pop();
        EXPECT_EQ(2, fifo.size());
        ASSERT_EQ(3, fifo.capacity());

        EXPECT_EQ(32, fifo.peek());
        fifo.pop();
        EXPECT_EQ(1, fifo.size());
        ASSERT_EQ(3, fifo.capacity());

        EXPECT_EQ(48, fifo.peek());
        fifo.pop();
        EXPECT_EQ(0, fifo.size());
        ASSERT_EQ(3, fifo.capacity());

        // push again so we over wright.
        fifo.push(64);
        EXPECT_EQ(1, fifo.size());
        ASSERT_EQ(3, fifo.capacity());

        EXPECT_EQ(64, fifo.peek());
        fifo.clear();
        EXPECT_EQ(0, fifo.size());
        ASSERT_EQ(3, fifo.capacity());
    }

    EXPECT_EQ(CustomType2::CONSTRUCTION_COUNT, CustomType2::DECONSTRUCTION_COUNT);
}

TYPED_TEST(FixedFifoTest, Iteration)
{
    { // scoped for con / de-con check
        FixedFifo<TypeParam, 3> fifo;

        EXPECT_EQ(0, fifo.size());
        ASSERT_EQ(3, fifo.capacity());

        fifo.push(16);
        fifo.push(32);
        fifo.push(48);

        fifo.pop();
        fifo.push(128);

        int numvalues = 0;
        TypeParam valueSum = 0;

        for (FixedFifo<TypeParam, 3>::iterator it = fifo.begin(); it != fifo.end(); ++it) {
            numvalues++;
            valueSum += (*it);
        }

        EXPECT_EQ(3, numvalues);
        EXPECT_EQ(208, valueSum);

        numvalues = 0;
        valueSum = 0;

        for (FixedFifo<TypeParam, 3>::const_iterator it = fifo.begin(); it != fifo.end(); ++it) {
            numvalues++;
            valueSum += (*it);
        }

        EXPECT_EQ(3, numvalues);
        EXPECT_EQ(208, valueSum);

        fifo.clear();
    }

    EXPECT_EQ(CustomType2::CONSTRUCTION_COUNT, CustomType2::DECONSTRUCTION_COUNT);
}

TYPED_TEST(FixedFifoTest, Iteration2)
{
    { // scoped for con / de-con check
        FixedFifo<TypeParam, 4> fifo;
        const auto& constFifo = fifo;

        EXPECT_EQ(0, fifo.size());
        ASSERT_EQ(4, fifo.capacity());

        fifo.push(16);
        fifo.push(32);
        fifo.push(64);
        fifo.push(128);

        int numvalues = 0;
        TypeParam valueSum = 0;

        for(auto& val : fifo) {
            numvalues++;
            valueSum += val;
        }

        EXPECT_EQ(4, numvalues);
        EXPECT_EQ(240, valueSum);

        numvalues = 0;
        valueSum = 0;

        for (auto& val : constFifo) {
            numvalues++;
            valueSum += val;
        }

        EXPECT_EQ(4, numvalues);
        EXPECT_EQ(240, valueSum);

        fifo.clear();
    }

    EXPECT_EQ(CustomType2::CONSTRUCTION_COUNT, CustomType2::DECONSTRUCTION_COUNT);
}

TEST(FixedFifoTest, WrapAround)
{
    // test inserting and pop so that we wrap around.
    constexpr size_t size = 4;
    constexpr size_t loops = 64;
    size_t value = 0;

    FixedFifo<size_t, size> fifo;

    ASSERT_EQ(size, fifo.capacity());

    for (size_t i = 0; i < size; i++) {
        fifo.push(value++);
        EXPECT_EQ(0, fifo.peek());
    }

    EXPECT_EQ(size, fifo.size());
    ASSERT_EQ(0, fifo.freeSpace());

    for (size_t i = 0; i < loops; i++)
    {
        EXPECT_EQ(value - size, fifo.peek());
        fifo.pop();
        EXPECT_EQ(value - (size - 1), fifo.peek());
        fifo.push(value++);
    }

    EXPECT_EQ(size, fifo.size());
    ASSERT_EQ(0, fifo.freeSpace());

    for (size_t i = 0; i < size; i++) {
        EXPECT_EQ(value - (size - i), fifo.peek());
        fifo.pop();
    }

    EXPECT_EQ(0, fifo.size());
    ASSERT_EQ(size, fifo.freeSpace());
}