#include "stdafx.h"

#include <Containers\FixedArray.h>

X_USING_NAMESPACE;

using namespace core;

typedef ::testing::Types<short, int, float> MyTypes;
TYPED_TEST_CASE(FixedArrayTest, MyTypes);

template<typename T>
class FixedArrayTest : public ::testing::Test
{
public:
};

namespace
{
    X_ALIGNED_SYMBOL(struct UserType, 64)
    {
    public:
        UserType(void) :
            val_(0)
        {
        }
        explicit UserType(int value) :
            val_(value)
        {
        }
        int val(void) const
        {
            return val_;
        }

    private:
        int val_;
        char unusedPadding[64 - sizeof(int)];
    };

    X_ALIGNED_SYMBOL(struct CustomTypeComplex, 64)
    {
        CustomTypeComplex(size_t val, const char* pName) :
            var_(val),
            pName_(pName)
        {
            CONSRUCTION_COUNT++;
        }
        CustomTypeComplex(const CustomTypeComplex& oth) :
            var_(oth.var_),
            pName_(oth.pName_)
        {
            ++CONSRUCTION_COUNT;
        }
        CustomTypeComplex(CustomTypeComplex && oth) :
            var_(oth.var_),
            pName_(oth.pName_)
        {
            ++MOVE_COUNT;
        }

        ~CustomTypeComplex()
        {
            DECONSRUCTION_COUNT++;
        }

        CustomTypeComplex& operator=(const CustomTypeComplex& val)
        {
            var_ = val.var_;
            return *this;
        }

        inline size_t GetVar(void) const
        {
            return var_;
        }
        inline const char* GetName(void) const
        {
            return pName_;
        }

    private:
        size_t var_;
        const char* pName_;

    public:
        static int CONSRUCTION_COUNT;
        static int MOVE_COUNT;
        static int DECONSRUCTION_COUNT;
    };

    int CustomTypeComplex::CONSRUCTION_COUNT = 0;
    int CustomTypeComplex::MOVE_COUNT = 0;
    int CustomTypeComplex::DECONSRUCTION_COUNT = 0;
} // namespace

TYPED_TEST(FixedArrayTest, BuiltInType)
{
    FixedArray<TypeParam, 32> array;

    ASSERT_EQ(32, array.capacity());
    ASSERT_EQ(0, array.size());

    for (FixedArray<TypeParam, 32>::size_type i = 0; i < array.capacity(); i++) {
        array.append(TypeParam());
    }

    ASSERT_EQ(32, array.size());

    for (FixedArray<TypeParam, 32>::size_type i = 0; i < array.size(); i++) {
        EXPECT_EQ(TypeParam(), array[i]);
    }

    for (FixedArray<TypeParam, 32>::iterator it = array.begin(); it != array.end(); ++it) {
        EXPECT_EQ(TypeParam(), *it);
        *it = TypeParam(5);
    }

    for (FixedArray<TypeParam, 32>::const_iterator it = array.begin(); it != array.end(); ++it) {
        EXPECT_EQ(TypeParam(5), *it);
    }

    TypeParam val = TypeParam(0);
    for (FixedArray<TypeParam, 32>::iterator it = array.begin(); it != array.end(); ++it) {
        *it = val;
        val++;
    }

    // remove
    array.removeIndex(3);
    array.removeIndex(20);
    array.remove(array.begin() + 15);

    ASSERT_EQ(32, array.capacity());
    ASSERT_EQ(29, array.size());

    // should of moved down.
    EXPECT_EQ(4, array[3]);
}

TYPED_TEST(FixedArrayTest, UserType)
{
    FixedArray<UserType, 32> array(UserType(1337));

    ASSERT_EQ(32, array.capacity());
    ASSERT_EQ(32, array.size());

    for (FixedArray<UserType, 32>::size_type i = 0; i < array.size(); i++) {
        EXPECT_EQ(1337, array[i].val());
    }

    for (FixedArray<UserType, 32>::iterator it = array.begin(); it != array.end(); ++it) {
        EXPECT_EQ(1337, it->val());
        *it = UserType(5);
    }

    for (FixedArray<UserType, 32>::const_iterator it = array.begin(); it != array.end(); ++it) {
        EXPECT_EQ(5, it->val());
    }

    // remove
    array.removeIndex(3);
    array.removeIndex(20);
    array.remove(array.begin() + 15);

    ASSERT_EQ(32, array.capacity());
    ASSERT_EQ(29, array.size());
}

TEST(FixedArrayTest, EmplaceBackComplex)
{
    FixedArray<CustomTypeComplex, 64> list;

    EXPECT_EQ(0, list.size());
    ASSERT_EQ(64, list.capacity());
    EXPECT_NE(nullptr, list.ptr());

    EXPECT_EQ(0, CustomTypeComplex::CONSRUCTION_COUNT);
    EXPECT_EQ(0, CustomTypeComplex::MOVE_COUNT);
    EXPECT_EQ(0, CustomTypeComplex::DECONSRUCTION_COUNT);

    for (int i = 0; i < 32; i++) {
        list.emplace_back(i * 4, "HEllo");
    }

    EXPECT_EQ(32, CustomTypeComplex::CONSRUCTION_COUNT);
    EXPECT_EQ(0, CustomTypeComplex::MOVE_COUNT);
    EXPECT_EQ(0, CustomTypeComplex::DECONSRUCTION_COUNT);

    for (int i = 32; i < 64; i++) {
        list.push_back(CustomTypeComplex(i * 4, "HEllo"));
    }

    EXPECT_EQ(64, CustomTypeComplex::CONSRUCTION_COUNT);
    EXPECT_EQ(32, CustomTypeComplex::MOVE_COUNT);
    EXPECT_EQ(32, CustomTypeComplex::DECONSRUCTION_COUNT);

    EXPECT_EQ(64, list.size());
    ASSERT_EQ(64, list.capacity());

    for (int i = 0; i < 64; i++) {
        EXPECT_EQ(i * 4, list[i].GetVar());
        EXPECT_STREQ("HEllo", list[i].GetName());
    }

    list.clear();

    EXPECT_EQ(0, list.size());
    ASSERT_EQ(64, list.capacity());
}

TEST(FixedArrayTest, AlignMentComplex)
{
    FixedArray<CustomTypeComplex, 64> array;

    for (int i = 0; i < 32; i++) {
        array.emplace_back(i * 4, "HEllo");
    }

    for (FixedArray<CustomTypeComplex, 64>::iterator it = array.begin(); it != array.end(); ++it) {
        X_ASSERT_ALIGNMENT(&(*it), X_ALIGN_OF(CustomTypeComplex), 0);
    }

    for (FixedArray<CustomTypeComplex, 64>::const_iterator it = array.begin(); it != array.end(); ++it) {
        X_ASSERT_ALIGNMENT(&(*it), X_ALIGN_OF(CustomTypeComplex), 0);
    }
}

TYPED_TEST(FixedArrayTest, front)
{
    FixedArray<TypeParam, 16> array;

    array.append(static_cast<TypeParam>(5));
    array.append(static_cast<TypeParam>(6));
    EXPECT_EQ(5, array.front());

    FixedArray<TypeParam, 16>::ConstReference constRef = array.front();

    EXPECT_EQ(5, constRef);
}

TYPED_TEST(FixedArrayTest, back)
{
    FixedArray<TypeParam, 16> array;

    array.append(static_cast<TypeParam>(5));
    array.append(static_cast<TypeParam>(6));
    EXPECT_EQ(6, array.back());

    FixedArray<TypeParam, 16>::ConstReference constRef = array.back();
    EXPECT_EQ(6, constRef);
}

X_PRAGMA(optimize("", off))

TYPED_TEST(FixedArrayTest, front_fail)
{
    FixedArray<TypeParam, 16> array;
    const FixedArray<TypeParam, 16> const_array;

    core::debugging::EnableBreakpoints(false);
    g_AssetChecker.ExpectAssertion(true);

    // should throw assert.
    array.front();
    EXPECT_TRUE(g_AssetChecker.HadCorrectAssertions());

    g_AssetChecker.ExpectAssertion(true);

    const_array.front();
    EXPECT_TRUE(g_AssetChecker.HadCorrectAssertions());

    g_AssetChecker.ExpectAssertion(false);
    core::debugging::EnableBreakpoints(true);
}

TYPED_TEST(FixedArrayTest, back_fail)
{
    FixedArray<TypeParam, 16> array;
    const FixedArray<TypeParam, 16> const_array;

    core::debugging::EnableBreakpoints(false);
    g_AssetChecker.ExpectAssertion(true);

    // should throw assert.
    array.back();
    EXPECT_TRUE(g_AssetChecker.HadCorrectAssertions());

    g_AssetChecker.ExpectAssertion(true);

    const_array.back();
    EXPECT_TRUE(g_AssetChecker.HadCorrectAssertions());

    g_AssetChecker.ExpectAssertion(false);
    core::debugging::EnableBreakpoints(true);
}

X_PRAGMA(optimize("", on))
