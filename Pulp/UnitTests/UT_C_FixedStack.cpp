#include "stdafx.h"

#include <gtest\gtest.h>

X_USING_NAMESPACE;

using namespace core;


typedef ::testing::Types<float, int> MyTypes;
TYPED_TEST_CASE(FixedStackTest, MyTypes);

template <typename T>
class FixedStackTest : public ::testing::Test {
public:
};

TYPED_TEST(FixedStackTest, DefaultTypes)
{
	FixedStack<TypeParam,16> stack;

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

}

namespace {

	X_ALIGNED_SYMBOL(struct CustomType2, 32)
	{
		CustomType2() : var_(16) {
			CONSRUCTION_COUNT++;
		}

		CustomType2(const CustomType2& oth)
			: var_(oth.var_)
		{
			++CONSRUCTION_COUNT;
		}

		~CustomType2() {
			DECONSRUCTION_COUNT++;
		}

		inline int GetVar() const {
			return var_;
		}
	private:
		int var_;

	public:
		static int CONSRUCTION_COUNT;
		static int DECONSRUCTION_COUNT;
	};

	int CustomType2::CONSRUCTION_COUNT = 0;
	int CustomType2::DECONSRUCTION_COUNT = 0;

}


TEST(FixedStackTest, CustomTypes)
{
	{
		FixedStack<CustomType2, 100> stack;
		CustomType2 val;

		EXPECT_EQ(0, stack.size());
		ASSERT_EQ(100, stack.capacity()); // must have room.

		// add some items baby.
		for (int i = 0; i < 90; i++)
			stack.push(val);

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

	}

	EXPECT_EQ(CustomType2::CONSRUCTION_COUNT, CustomType2::DECONSRUCTION_COUNT);
}

