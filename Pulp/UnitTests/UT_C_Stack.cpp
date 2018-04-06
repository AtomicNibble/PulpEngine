#include "stdafx.h"

#include <stack>

X_USING_NAMESPACE;

using namespace core;

typedef ::testing::Types<float, int> MyTypes;
TYPED_TEST_CASE(StackTest, MyTypes);

template<typename T>
class StackTest : public ::testing::Test
{
public:
};

namespace
{
    X_ALIGNED_SYMBOL(struct CustomType, 32)
    {
        CustomType() :
            var_(16),
            pStr_(nullptr)
        {
            CONSRUCTION_COUNT++;
        }

        CustomType(int val, const char* pStr) :
            var_(val),
            pStr_(pStr)
        {
            CONSRUCTION_COUNT++;
        }

        CustomType(const CustomType& oth) :
            var_(oth.var_),
            pStr_(oth.pStr_)
        {
            ++CONSRUCTION_COUNT;
        }

        CustomType(const CustomType&& oth) :
            var_(oth.var_),
            pStr_(oth.pStr_)
        {
            ++MOVE_COUNT;
        }

        ~CustomType()
        {
            DECONSRUCTION_COUNT++;
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

    int CustomType::MOVE_COUNT = 0;
    int CustomType::CONSRUCTION_COUNT = 0;
    int CustomType::DECONSRUCTION_COUNT = 0;

} // namespace

TYPED_TEST(StackTest, DefaultTypes)
{
    Stack<TypeParam> stack(g_arena);

    EXPECT_EQ(0, stack.size());
    EXPECT_EQ(0, stack.capacity());

    stack.reserve(16);

    EXPECT_EQ(0, stack.size());
    ASSERT_EQ(16, stack.capacity()); // must have room.

    // add some items baby.
    for (int i = 0; i < 16; i++)
        stack.push((TypeParam)i + 1);

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

    stack.free();

    EXPECT_EQ(0, stack.size());
    EXPECT_EQ(0, stack.capacity());
}

TEST(StackTest, CustomTypes)
{
    CustomType::MOVE_COUNT = 0;
    CustomType::DECONSRUCTION_COUNT = 0;
    CustomType::CONSRUCTION_COUNT = 0;

    Stack<CustomType> stack(g_arena);
    {
        CustomType val;

        EXPECT_EQ(0, stack.size());
        EXPECT_EQ(0, stack.capacity());

        stack.reserve(100);

        EXPECT_EQ(0, stack.size());
        ASSERT_EQ(100, stack.capacity()); // must have room.

        // add some items baby.
        for (int i = 0; i < 90; i++) {
            stack.push(val);
        }

        EXPECT_EQ(90, stack.size());
        EXPECT_EQ(100, stack.capacity());

        for (int i = 90; i > 10; i--) {
            EXPECT_EQ(16, stack.top().GetVar());
            stack.pop();
        }

        EXPECT_EQ(10, stack.size());
        EXPECT_EQ(100, stack.capacity()); // should be unchanged.

        stack.clear();

        EXPECT_EQ(0, stack.size());
        EXPECT_EQ(100, stack.capacity()); // should be unchanged.

        stack.free();

        EXPECT_EQ(0, stack.size());
        EXPECT_EQ(0, stack.capacity()); // should be unchanged.
    }

    size_t m = CustomType::MOVE_COUNT;
    size_t d = CustomType::DECONSRUCTION_COUNT;
    size_t c = CustomType::CONSRUCTION_COUNT;

    EXPECT_EQ(0, CustomType::MOVE_COUNT);
    EXPECT_EQ(91, CustomType::DECONSRUCTION_COUNT);
    EXPECT_EQ(91, CustomType::CONSRUCTION_COUNT);
}

TEST(StackTest, CustomTypesDecon) // check leaving scope cleans up correct.
{
    CustomType::MOVE_COUNT = 0;
    CustomType::DECONSRUCTION_COUNT = 0;
    CustomType::CONSRUCTION_COUNT = 0;

    {
        Stack<CustomType> stack(g_arena);
        {
            CustomType val;

            EXPECT_EQ(0, stack.size());
            EXPECT_EQ(0, stack.capacity());

            stack.reserve(100);

            EXPECT_EQ(0, stack.size());
            ASSERT_EQ(100, stack.capacity()); // must have room.

            // add some items baby.
            for (int i = 0; i < 90; i++) {
                stack.push(val);
            }

            EXPECT_EQ(90, stack.size());
            EXPECT_EQ(100, stack.capacity());

            for (int i = 90; i > 10; i--) {
                EXPECT_EQ(16, stack.top().GetVar());
                stack.pop();
            }

            EXPECT_EQ(10, stack.size());
            EXPECT_EQ(100, stack.capacity()); // should be unchanged.

            // should clear itself up.
        }
    }

    size_t m = CustomType::MOVE_COUNT;
    size_t d = CustomType::DECONSRUCTION_COUNT;
    size_t c = CustomType::CONSRUCTION_COUNT;

    EXPECT_EQ(0, CustomType::MOVE_COUNT);
    EXPECT_EQ(91, CustomType::DECONSRUCTION_COUNT);
    EXPECT_EQ(91, CustomType::CONSRUCTION_COUNT);
}

TEST(StackTest, Emplace)
{
    CustomType::MOVE_COUNT = 0;
    CustomType::DECONSRUCTION_COUNT = 0;
    CustomType::CONSRUCTION_COUNT = 0;

    Stack<CustomType> stack(g_arena);
    {
        CustomType val;

        EXPECT_EQ(0, stack.size());
        EXPECT_EQ(0, stack.capacity());

        stack.reserve(100);

        EXPECT_EQ(0, stack.size());
        ASSERT_EQ(100, stack.capacity()); // must have room.

        // add some items baby.
        for (int i = 0; i < 90; i++) {
            stack.emplace(0x400, "meow");
        }

        EXPECT_EQ(90, stack.size());
        EXPECT_EQ(100, stack.capacity());

        for (int i = 90; i > 10; i--) {
            EXPECT_EQ(0x400, stack.top().GetVar());
            EXPECT_STREQ("meow", stack.top().GetName());
            stack.pop();
        }

        EXPECT_EQ(10, stack.size());
        EXPECT_EQ(100, stack.capacity()); // should be unchanged.

        stack.clear();

        EXPECT_EQ(0, stack.size());
        EXPECT_EQ(100, stack.capacity()); // should be unchanged.

        stack.free();

        EXPECT_EQ(0, stack.size());
        EXPECT_EQ(0, stack.capacity()); // should be unchanged.
    }

    EXPECT_EQ(0, CustomType::MOVE_COUNT);
    EXPECT_EQ(91, CustomType::DECONSRUCTION_COUNT);
    EXPECT_EQ(91, CustomType::CONSRUCTION_COUNT);
}

TEST(StackTest, Move)
{
    CustomType::MOVE_COUNT = 0;
    CustomType::DECONSRUCTION_COUNT = 0;
    CustomType::CONSRUCTION_COUNT = 0;

    Stack<CustomType> stack(g_arena);
    {
        EXPECT_EQ(0, stack.size());
        EXPECT_EQ(0, stack.capacity());

        stack.reserve(100);

        EXPECT_EQ(0, stack.size());
        ASSERT_EQ(100, stack.capacity()); // must have room.

        // add some items baby.
        for (int i = 0; i < 90; i++) {
            stack.emplace(CustomType(0x400, "meow"));
        }

        EXPECT_EQ(90, stack.size());
        EXPECT_EQ(100, stack.capacity());

        for (int i = 90; i > 10; i--) {
            EXPECT_EQ(0x400, stack.top().GetVar());
            EXPECT_STREQ("meow", stack.top().GetName());
            stack.pop();
        }

        EXPECT_EQ(10, stack.size());
        EXPECT_EQ(100, stack.capacity()); // should be unchanged.

        stack.clear();

        EXPECT_EQ(0, stack.size());
        EXPECT_EQ(100, stack.capacity()); // should be unchanged.

        stack.free();

        EXPECT_EQ(0, stack.size());
        EXPECT_EQ(0, stack.capacity()); // should be unchanged.
    }

    EXPECT_EQ(90, CustomType::MOVE_COUNT);
    EXPECT_EQ(90, CustomType::CONSRUCTION_COUNT);
    EXPECT_EQ(180, CustomType::DECONSRUCTION_COUNT);
}

TEST(StackTest, AlignMentCustom)
{
    FixedStack<CustomType, 16> stack;

    for (int i = 0; i < 16; i++) {
        stack.emplace(CustomType(0x400, "meow"));
    }

    for (FixedStack<CustomType, 16>::const_iterator it = stack.begin(); it != stack.end(); ++it) {
        X_ASSERT_ALIGNMENT(&(*it), X_ALIGN_OF(CustomType), 0);
    }
}

TYPED_TEST(StackTest, reserve)
{
    Stack<TypeParam> stack(g_arena);

    EXPECT_EQ(0, stack.size());
    EXPECT_EQ(0, stack.capacity());

    stack.reserve(64);

    EXPECT_EQ(0, stack.size());
    ASSERT_EQ(64, stack.capacity());

    stack.reserve(40);
    // should still be same size
    EXPECT_EQ(0, stack.size());
    ASSERT_EQ(64, stack.capacity());

    stack.push(0x1337);

    ASSERT_EQ(1, stack.size());
    ASSERT_EQ(64, stack.capacity());

    stack.reserve(128);

    ASSERT_EQ(1, stack.size());
    ASSERT_EQ(128, stack.capacity());

    EXPECT_EQ(0x1337, stack.top());
}

TYPED_TEST(StackTest, Iterator)
{
    Stack<TypeParam> list(g_arena);

    list.reserve(64);

    for (size_t i = 0; i < 64; i++) {
        list.push(128);
    }

    Stack<TypeParam>::Iterator it = list.begin();
    for (; it != list.end(); ++it) {
        EXPECT_EQ(128, *it);
        *it = 64;
    }

    Stack<TypeParam>::ConstIterator cit = list.begin();
    for (; cit != list.end(); ++cit) {
        EXPECT_EQ(64, *cit);
    }
}

TYPED_TEST(StackTest, AlignMent)
{
    FixedStack<TypeParam, 16> stack;

    for (int i = 0; i < 16; i++) {
        stack.push(0x173263);
    }

    for (FixedStack<TypeParam, 16>::const_iterator it = stack.begin(); it != stack.end(); ++it) {
        X_ASSERT_ALIGNMENT(&(*it), X_ALIGN_OF(TypeParam), 0);
    }
}