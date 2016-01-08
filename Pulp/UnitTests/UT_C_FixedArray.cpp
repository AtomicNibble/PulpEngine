#include "stdafx.h"



#include <Containers\FixedArray.h>

X_USING_NAMESPACE;

using namespace core;


typedef ::testing::Types<short, int, float> MyTypes;
TYPED_TEST_CASE(FixedArrayTest, MyTypes);

template <typename T>
class FixedArrayTest : public ::testing::Test {
public:
};

namespace
{
	X_ALIGNED_SYMBOL(struct UserType, 64)
	{
	public:
		UserType(void) : val_(0) {}
		explicit UserType(int value) : val_(value) {}
		int val(void) const { return val_; }

	private:
		int val_;
		char unusedPadding[64 - sizeof(int)];
	};
}



TYPED_TEST(FixedArrayTest, BuiltInType)
{
	FixedArray<TypeParam,32> array;

	ASSERT_EQ(32, array.capacity());
	ASSERT_EQ(0, array.size());

	for (FixedArray<TypeParam, 32>::size_type i = 0; i < array.capacity(); i++)
	{
		array.append(TypeParam());
	}

	ASSERT_EQ(32, array.size());


	for (FixedArray<TypeParam, 32>::size_type i = 0; i < array.size(); i++)
	{
		EXPECT_EQ(TypeParam(), array[i]);
	}

	for (FixedArray<TypeParam, 32>::iterator it = array.begin(); it != array.end(); ++it)
	{
		EXPECT_EQ(TypeParam(), *it);
		*it = TypeParam(5);
	}

	for (FixedArray<TypeParam, 32>::const_iterator it = array.begin(); it != array.end(); ++it)
	{
		EXPECT_EQ(TypeParam(5), *it);
	}

	TypeParam val = TypeParam(0);
	for (FixedArray<TypeParam, 32>::iterator it = array.begin(); it != array.end(); ++it)
	{
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

	for (FixedArray<UserType, 32>::size_type i = 0; i < array.size(); i++)
	{
		EXPECT_EQ(1337, array[i].val());
	}

	for (FixedArray<UserType, 32>::iterator it = array.begin(); it != array.end(); ++it)
	{
		EXPECT_EQ(1337, it->val());
		*it = UserType(5);
	}

	for (FixedArray<UserType, 32>::const_iterator it = array.begin(); it != array.end(); ++it)
	{
		EXPECT_EQ(5, it->val());
	}

	// remove
	array.removeIndex(3);
	array.removeIndex(20);
	array.remove(array.begin() + 15);

	ASSERT_EQ(32, array.capacity());
	ASSERT_EQ(29, array.size());
}