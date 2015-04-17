#include "stdafx.h"
#include "gtest/gtest.h"

#include <String\StackString.h>

X_USING_NAMESPACE;


TEST(StackString, Construct)
{
	core::StackString<32> str32_empty;
	core::StackString<32> str32_str("hello bob");
	core::StackString<32> str32_bool(true);
	core::StackString<32> str32_char('a');
	core::StackString<32> str32_int(-101);
	core::StackString<32> str32_uint(101u);
	core::StackString<32> str32_float(0.404054f);
	core::StackString<32> str32_64(123456789123456454);
	core::StackString<32> str32_u64(123456789123456454ull);
	core::StackString<32> str32_startEnd(str32_str.begin(), str32_str.end());

	core::StringRange range(str32_str.begin(), str32_str.end());

	core::StackString<32> str32_range(range);


	EXPECT_STREQ("", str32_empty.c_str());
	EXPECT_STREQ("hello bob", str32_str.c_str());
	EXPECT_STREQ("1", str32_bool.c_str());
	EXPECT_STREQ("a", str32_char.c_str());
	EXPECT_STREQ("-101", str32_int.c_str());
	EXPECT_STREQ("101", str32_uint.c_str());
	EXPECT_STREQ("0.404054", str32_float.c_str());
	EXPECT_STREQ("123456789123456454", str32_64.c_str());
	EXPECT_STREQ("123456789123456454", str32_u64.c_str());
	EXPECT_STREQ("hello bob", str32_startEnd.c_str());
	EXPECT_STREQ("hello bob", str32_range.c_str());

}