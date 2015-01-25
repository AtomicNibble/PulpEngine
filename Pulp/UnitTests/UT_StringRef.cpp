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
	EXPECT_STREQ("", empty.c_str());
	EXPECT_STREQ("", empty.data());
	EXPECT_EQ(empty.begin(), empty.end());

	StrRefT cstr("test");

	EXPECT_FALSE(cstr.isEmpty());
	EXPECT_TRUE(cstr.isNotEmpty());
	EXPECT_EQ(4, cstr.length());
	EXPECT_EQ(4, cstr.capacity());
	EXPECT_EQ(cstr.begin() + 4, cstr.end());
	EXPECT_STRNE("", cstr.c_str());
	EXPECT_STRNE("", cstr.data());
	EXPECT_STREQ("test", cstr.c_str());

	StrRefT fromoth(cstr);
	 
	EXPECT_FALSE(fromoth.isEmpty());
	EXPECT_TRUE(fromoth.isNotEmpty());
	EXPECT_EQ(4, fromoth.length());
	EXPECT_EQ(4, fromoth.capacity());
	EXPECT_EQ(fromoth.begin() + 4, fromoth.end());
	EXPECT_STRNE("", fromoth.c_str());
	EXPECT_STRNE("", fromoth.data());
	EXPECT_STREQ("test", fromoth.c_str());


	StrRefT fchar('T');

	EXPECT_FALSE(fchar.isEmpty());
	EXPECT_TRUE(fchar.isNotEmpty());
	EXPECT_EQ(1, fchar.length());
	EXPECT_EQ(1, fchar.capacity());
	EXPECT_EQ(fchar.begin() + 1, fchar.end());
	EXPECT_STRNE("", fchar.c_str());
	EXPECT_STRNE("", fchar.data());
	EXPECT_STREQ("T", fchar.c_str());

	StrRefT fcharNum('T', 32);

	EXPECT_FALSE(fcharNum.isEmpty());
	EXPECT_TRUE(fcharNum.isNotEmpty());
	EXPECT_EQ(32, fcharNum.length());
	EXPECT_EQ(32, fcharNum.capacity());
	EXPECT_EQ(fcharNum.begin() + 32, fcharNum.end());
	EXPECT_EQ('T', fcharNum[16]);
	EXPECT_STRNE("", fcharNum.c_str());
	EXPECT_STRNE("", fcharNum.data());
	EXPECT_STREQ("TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT", fcharNum.c_str());

	StrRefT fstrlen("tickle a cat", 6);

	EXPECT_FALSE(fstrlen.isEmpty());
	EXPECT_TRUE(fstrlen.isNotEmpty());
	EXPECT_EQ(6, fstrlen.length());
	EXPECT_EQ(6, fstrlen.capacity());
	EXPECT_EQ(fstrlen.begin() + 6, fstrlen.end());
	EXPECT_EQ('c', fstrlen[2]);
	EXPECT_STRNE("", fstrlen.c_str()); 
	EXPECT_STRNE("", fstrlen.data());
	EXPECT_STREQ("tickle", fstrlen.c_str());

	const char* begin = "goat on a boat";
	const char* end = begin + strlen(begin);
	StrRefT fbeginend(begin,end);

	EXPECT_FALSE(fbeginend.isEmpty());
	EXPECT_TRUE(fbeginend.isNotEmpty());
	EXPECT_EQ(14, fbeginend.length());
	EXPECT_EQ(14, fbeginend.capacity());
	EXPECT_EQ(fbeginend.begin() + 14, fbeginend.end());
	EXPECT_EQ('a', fbeginend[2]);
	EXPECT_STRNE("", fbeginend.c_str());
	EXPECT_STRNE("", fbeginend.data());
	EXPECT_STREQ("goat on a boat", fbeginend.c_str());

}


TYPED_TEST(StrRef, Resize)
{
	typedef StringRef<TypeParam> StrRefT;

	StrRefT str("tickle a cat");

	EXPECT_EQ(12, str.length());

	str.resize(3);

	EXPECT_EQ(3, str.length());

	str.resize(6);

	EXPECT_EQ(6, str.length());
	EXPECT_EQ(' ', str[5]);

	str.resize(10, '_');

	EXPECT_EQ(10, str.length());
	EXPECT_EQ('_', str[8]);
}


TYPED_TEST(StrRef, Case)
{
	typedef StringRef<TypeParam> StrRefT;

	StrRefT str("tickle a cat");
	
	str.toUpper();
	EXPECT_STREQ("TICKLE A CAT", str);
	str.toLower();
	EXPECT_STREQ("tickle a cat", str);

	StrRefT str1("HeLlO");

	str1.toLower();
	EXPECT_STREQ("hello", str1);
	str1.toUpper();
	EXPECT_STREQ("HELLO", str1);
}

TYPED_TEST(StrRef, TrimLeft)
{
	typedef StringRef<TypeParam> StrRefT;

	StrRefT str("       tickle a cat     ");
	str.trimLeft();
	EXPECT_STREQ("tickle a cat     ", str);

	StrRefT str1("      _ tickle a cat     ");
	str1.trimLeft(' ');
	EXPECT_STREQ("_ tickle a cat     ", str1);

	StrRefT str2("      _ tickle a cat     ");
	str2.trimLeft("_ ");
	EXPECT_STREQ("tickle a cat     ", str2);
}

TYPED_TEST(StrRef, TrimRight)
{
	typedef StringRef<TypeParam> StrRefT;

	StrRefT str("       tickle a cat     ");
	str.trimRight();
	EXPECT_STREQ("       tickle a cat", str);

	StrRefT str1("      tickle a cat  _____");
	str1.trimRight('_');
	EXPECT_STREQ("      tickle a cat  ", str1);

	StrRefT str2("      tickle a cat   _  ");
	str2.trimRight("cat _");
	EXPECT_STREQ("      tickle", str2);
}

TYPED_TEST(StrRef, Trim)
{
	typedef StringRef<TypeParam> StrRefT;

	StrRefT str("       tickle a cat     ");
	str.trim();
	EXPECT_STREQ("tickle a cat", str);

	StrRefT str1("   _   tickle a cat  _   ");
	str1.trim('_');
	EXPECT_STREQ("   _   tickle a cat  _   ", str1);
	str1.trim(' ');
	EXPECT_STREQ("_   tickle a cat  _", str1);
	str1.trim('_');
	EXPECT_STREQ("   tickle a cat  ", str1);


	StrRefT str2("      tickle a cat   _  ");
	str2.trim("cat _");
	EXPECT_STREQ("ickle", str2);
	str2.trim();
	EXPECT_STREQ("ickle", str2);
}


TYPED_TEST(StrRef, Append)
{



}

TYPED_TEST(StrRef, Assign)
{



}

TYPED_TEST(StrRef, Replace)
{



}

TYPED_TEST(StrRef, Insert)
{



}

TYPED_TEST(StrRef, Erase)
{



}

TYPED_TEST(StrRef, Compare)
{



}

TYPED_TEST(StrRef, Swap)
{



}

TYPED_TEST(StrRef, SubStr)
{



}

TYPED_TEST(StrRef, Left)
{



}

TYPED_TEST(StrRef, Right)
{



}


TYPED_TEST(StrRef, OPAssign)
{



}

TYPED_TEST(StrRef, OPConcat)
{



}

