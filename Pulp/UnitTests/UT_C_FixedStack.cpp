#include "stdafx.h"

X_USING_NAMESPACE;

using namespace core;

typedef ::testing::Types<float, int> MyTypes;
TYPED_TEST_CASE(FixedStackTest, MyTypes);

template<typename T>
class FixedStackTest : public ::testing::Test
{
public:
};

namespace
{
    X_ALIGNED_SYMBOL(struct CustomType2, 32)
    {
        CustomType2(int var, const char* pStr) :
            var_(var),
            pStr_(pStr)
        {
            CONSRUCTION_COUNT++;
        }

        CustomType2(const CustomType2& oth) :
            var_(oth.var_),
            pStr_(oth.pStr_)
        {
            ++CONSRUCTION_COUNT;
        }

        CustomType2(CustomType2 && oth) :
            var_(oth.var_),
            pStr_(oth.pStr_)
        {
            ++MOVE_COUNT;
        }

        ~CustomType2()
        {
            DECONSRUCTION_COUNT++;
        }

        inline void SetVar(int var)
        {
            var_ = var;
        }
        inline int GetVar(void) const
        {
            return var_;
        }
        inline const char* GetName(void) const
        {
            return pStr_;
        }

    private:
        int var_;
        const char* pStr_;

    public:
        static int MOVE_COUNT;
        static int CONSRUCTION_COUNT;
        static int DECONSRUCTION_COUNT;
    };

    int CustomType2::MOVE_COUNT = 0;
    int CustomType2::CONSRUCTION_COUNT = 0;
    int CustomType2::DECONSRUCTION_COUNT = 0;

} // namespace

TYPED_TEST(FixedStackTest, DefaultTypes)
{
    FixedStack<TypeParam, 16> stack;

    EXPECT_EQ(0, stack.size());
    ASSERT_EQ(16, stack.capacity()); // must have room.

    // add some items baby.
    for (int i = 0; i < 16; i++) {
        stack.push((TypeParam)i + 1);
    }

    EXPECT_EQ(16, stack.size());
    EXPECT_EQ(16, stack.top());
    EXPECT_EQ(16, stack.size()); // make sure top didnt remvoe a item.

    for (int i = 16; i > 10; i--) {
        EXPECT_EQ(i, stack.top());
        stack.pop();
    }

    EXPECT_EQ(10, stack.size());

    stack.clear();

    EXPECT_EQ(0, stack.size());
    EXPECT_EQ(16, stack.capacity());
}

TYPED_TEST(FixedStackTest, Iterator)
{
    FixedStack<TypeParam, 16> stack;

    for (int i = 0; i < 16; i++) {
        stack.push((TypeParam)i + 1);
    }

    size_t numValues = 0;
    for (FixedStack<TypeParam, 16>::iterator it = stack.begin(); it != stack.end(); ++it) {
        numValues++;
    }

    ASSERT_EQ(16, numValues);

    numValues = 0;
    for (FixedStack<TypeParam, 16>::const_iterator it = stack.begin(); it != stack.end(); ++it) {
        numValues++;
    }

    ASSERT_EQ(16, numValues);
}

TEST(FixedStackTest, CustomTypes)
{
    CustomType2::MOVE_COUNT = 0;
    CustomType2::CONSRUCTION_COUNT = 0;
    CustomType2::DECONSRUCTION_COUNT = 0;

    {
        FixedStack<CustomType2, 100> stack;

        // nothting should be constructed just the stack memory allocated.
        EXPECT_EQ(0, CustomType2::MOVE_COUNT);
        EXPECT_EQ(0, CustomType2::CONSRUCTION_COUNT);
        EXPECT_EQ(0, CustomType2::DECONSRUCTION_COUNT);

        EXPECT_EQ(0, stack.size());
        ASSERT_EQ(100, stack.capacity()); // must have room.

        // add some items baby, should use move constructor.
        for (int i = 0; i < 50; i++) {
            stack.push(CustomType2(0x173, "meow"));
        }

        EXPECT_EQ(50, stack.size());
        EXPECT_EQ(100, stack.capacity());

        EXPECT_EQ(50, CustomType2::MOVE_COUNT);
        EXPECT_EQ(50, CustomType2::CONSRUCTION_COUNT);
        EXPECT_EQ(50, CustomType2::DECONSRUCTION_COUNT);

        // add some with copy constructor.
        CustomType2 val(0x173, "meow");
        for (int i = 0; i < 20; i++) {
            stack.push(val);
        }

        EXPECT_EQ(70, stack.size());
        EXPECT_EQ(100, stack.capacity());

        EXPECT_EQ(50, CustomType2::MOVE_COUNT);
        EXPECT_EQ(50 + 20 + 1, CustomType2::CONSRUCTION_COUNT);
        EXPECT_EQ(50 + 0, CustomType2::DECONSRUCTION_COUNT);

        // add some with args.
        for (int i = 0; i < 20; i++) {
            stack.emplace(0x173, "meow");
        }

        EXPECT_EQ(90, stack.size());
        EXPECT_EQ(100, stack.capacity());

        EXPECT_EQ(50, CustomType2::MOVE_COUNT);
        EXPECT_EQ(50 + 20 + 1 + 20, CustomType2::CONSRUCTION_COUNT);
        EXPECT_EQ(50 + 0, CustomType2::DECONSRUCTION_COUNT);

        // check values
        for (int i = 0; i < 80; i++) {
            CustomType2& mute = stack.top();
            EXPECT_EQ(0x173, mute.GetVar());
            EXPECT_STREQ("meow", mute.GetName());
            mute.SetVar(0x5363);

            const CustomType2& unMute = stack.top();
            EXPECT_EQ(0x5363, unMute.GetVar());
            EXPECT_STREQ("meow", unMute.GetName());

            stack.pop();
        }

        // 10 left.
        EXPECT_EQ(10, stack.size());
        EXPECT_EQ(100, stack.capacity()); // should be unchanged.

        EXPECT_EQ(50, CustomType2::MOVE_COUNT);
        EXPECT_EQ(50 + 20 + 1 + 20, CustomType2::CONSRUCTION_COUNT);
        EXPECT_EQ(50 + 0 + 80, CustomType2::DECONSRUCTION_COUNT);

        stack.clear();

        EXPECT_EQ(0, stack.size());
        EXPECT_EQ(100, stack.capacity()); // should be unchanged.

        EXPECT_EQ(50, CustomType2::MOVE_COUNT);
        EXPECT_EQ(50 + 20 + 1 + 20, CustomType2::CONSRUCTION_COUNT);
        EXPECT_EQ(50 + 0 + 80 + 10, CustomType2::DECONSRUCTION_COUNT);
    }

    EXPECT_EQ(CustomType2::CONSRUCTION_COUNT + CustomType2::MOVE_COUNT, CustomType2::DECONSRUCTION_COUNT);
}

TEST(FixedStackTest, AlignMent)
{
    FixedStack<CustomType2, 16> stack;

    for (int i = 0; i < 16; i++) {
        stack.emplace(0x173263, "meow");
    }

    for (FixedStack<CustomType2, 16>::const_iterator it = stack.begin(); it != stack.end(); ++it) {
        X_ASSERT_ALIGNMENT(&(*it), 32, 0);
    }
}