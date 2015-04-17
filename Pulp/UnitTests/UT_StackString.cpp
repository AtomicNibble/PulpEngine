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

TEST(StackString, Replace)
{
	core::StackString<1024> str("tickle my pickle, i love a good tickle. my my");

	EXPECT_EQ(45, str.length());

	str.replace("tickle", "touch");
	EXPECT_STREQ("touch my pickle, i love a good tickle. my my", str.c_str());
	EXPECT_EQ(44, str.length());

	str.replace(str.begin() + 15, "my", "goat");
	EXPECT_STREQ("touch my pickle, i love a good tickle. goat my", str.c_str());

	str.replace('o', 'x');
	EXPECT_STREQ("txuch my pickle, i love a good tickle. goat my", str.c_str());

	str.replaceAll('o', 'x');
	EXPECT_STREQ("txuch my pickle, i lxve a gxxd tickle. gxat my", str.c_str());

	str.replaceAll("my", "pie");
	EXPECT_STREQ("txuch pie pickle, i lxve a gxxd tickle. gxat pie", str.c_str());

}

TEST(StackString, TrimWhiteSpace)
{
	core::StackString<1024> str("   show me your whitespace    ");

	str.trimWhitespace();
	EXPECT_STREQ("show me your whitespace", str.c_str());
}

TEST(StackString, trimCharacter)
{
	core::StackString<1024> str("--- -----show me your whitespace------");

	str.trimCharacter('-');
	EXPECT_STREQ(" -----show me your whitespace", str.c_str());
}

TEST(StackString, trimRight)
{
	core::StackString<1024> str("--- -----show me your whitespace------");

	str.trimRight('w');
	EXPECT_STREQ("--- -----sho", str.c_str());

	str.trimRight(str.c_str() + 10);
	EXPECT_STREQ("--- -----s", str.c_str());
}
