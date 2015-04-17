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


TEST(StackString, Set)
{
	core::StackString<1024> str("goat man");

	str.set("Lorem Ipsum is simply dummy text of the printing and"
		" typesetting industry.Lorem Ipsum has been the industry's"
		" standard dummy text ever since the 1500");

	EXPECT_STREQ("Lorem Ipsum is simply dummy text of the printing and"
		" typesetting industry.Lorem Ipsum has been the industry's"
		" standard dummy text ever since the 1500", str.c_str());

	const char testTxt[38] = { "slap a goat in a moat while in a boat" };

	str.set(testTxt, testTxt + 38);

	EXPECT_STREQ(testTxt, str.c_str());
}


TEST(StackString, Append)
{
	core::StackString<1024> str("camel man ");

	str.append('a', 5);
	EXPECT_STREQ("camel man aaaaa", str.c_str());

	str.append(" goat");
	EXPECT_STREQ("camel man aaaaa goat", str.c_str());

	str.append(" cat", 3);
	EXPECT_STREQ("camel man aaaaa goat ca", str.c_str());

	const char testTxt[10] = { "hookersss" };
	str.append(testTxt, testTxt + 10);
	EXPECT_STREQ("camel man aaaaa goat cahookersss", str.c_str());
}

TEST(StackString, AppendFmt)
{
	core::StackString<1024> str(" ");

	str.appendFmt("%i%s_%u", 1337, "shit", 0x4096);

	EXPECT_STREQ(" 1337shit_16534", str.c_str());

}
