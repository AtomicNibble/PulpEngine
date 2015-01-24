#include "stdafx.h"

#include "gtest/gtest.h"

#include <String\StrRef.h>

X_USING_NAMESPACE;

using namespace core;

typedef ::testing::Types<char> MyTypes;
TYPED_TEST_CASE(StrRef, MyTypes);

template <typename T>
class StrRef : public ::testing::Test {
public:
};


TYPED_TEST(StrRef, Construct)
{
	typedef StringRef<TypeParam> StrRefT;
	typedef TypeParam T;


	StrRefT empty;

	EXPECT_TRUE(empty.isEmpty());
	EXPECT_FALSE(empty.isNotEmpty());
	EXPECT_EQ(0, empty.length());
	EXPECT_EQ(0, empty.capacity());
	EXPECT_EQ(nullptr, empty.c_str());
	EXPECT_EQ(nullptr, empty.data());
	EXPECT_EQ(empty.begin(), empty.end());
}

