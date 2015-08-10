#include "stdafx.h"

#include "gtest/gtest.h"

#include <String\StringUtil.h>
#include <String\StackString.h>
#include <String\StringHash.h>


X_USING_NAMESPACE;

using namespace core;


TEST(StringUtil, StringBytes) {

	std::string str = "how long is this txt?";
	std::wstring strW = L"how long is this txt?";
	core::string cstr = "how long is this txt?"; 
	core::StringRef<wchar_t> cstrW = L"how long is this txt?";

	EXPECT_EQ(21, strUtil::StringBytes(str));
	EXPECT_EQ(21 * 2, strUtil::StringBytes(strW));
	EXPECT_EQ(21, strUtil::StringBytes(cstr));
	EXPECT_EQ(21 * 2, strUtil::StringBytes(cstrW));
}

TEST(StringUtil, StringBytesNUll) {

	std::string str = "how long is this txt?";
	std::wstring strW = L"how long is this txt?";
	core::string cstr = "how long is this txt?";
	core::StringRef<wchar_t> cstrW = L"how long is this txt?";

	EXPECT_EQ(22, strUtil::StringBytesIncNull(str));
	EXPECT_EQ(22 * 2, strUtil::StringBytesIncNull(strW));
	EXPECT_EQ(22, strUtil::StringBytesIncNull(cstr));
	EXPECT_EQ(22 * 2, strUtil::StringBytesIncNull(cstrW));
}

TEST(StringUtil, Find) {

	StackString512 str("my name is bob. Hello jane.");
	StackString512 str2("Hell");

	const char* find = strUtil::Find(str.begin(), str.end(), "Hello");
	const char* findNCI = strUtil::Find(str.begin(), str.end(), "hello");
	const char* findCI = strUtil::FindCaseInsensitive(str.begin(), str.end(), "hello");
	const char* fin_fail = strUtil::FindCaseInsensitive(str2.begin(), str2.end(), "hello");
	
	EXPECT_TRUE(find == (str.begin() + 16));
	EXPECT_FALSE(findNCI == (str.begin() + 16)); // should not find as case is diffrent
	EXPECT_TRUE(findCI == (str.begin() + 16));
	EXPECT_TRUE(fin_fail == nullptr); // should not find as case is diffrent
}


TEST(StringUtil, Find2) 
{
	// had a bug where find was returning true after only matching 2 chars.
	// this test checks that is fixed.
	StackString512 str("my Hell name is bob. Hello jane.");

	const char* find = strUtil::Find(str.begin(), str.end(), "Hello");
	const char* findCI = strUtil::FindCaseInsensitive(str.begin(), str.end(), "hello");

	// check it was second one.
	EXPECT_TRUE(find == (str.begin() + 21));
	EXPECT_TRUE(findCI == (str.begin() + 21));
}


TEST(StringUtil, WhiteSpace) {

	// check every char.
	const char min = std::numeric_limits<char>::lowest();
	const char max = std::numeric_limits<char>::max();

	for (int c = min; c <= max; ++c)
	{
		bool is_White =
			c == 0x20 ||	// SPACE(codepoint 32, U + 0020)
			c == 0x9 ||		// TAB(codepoint 9, U + 0009)
			c == 0xa ||		// LINE FEED(codepoint 10, U + 000A)
			c == 0xb ||		// LINE TABULATION(codepoint 11, U + 000B)
			c == 0xc ||		// FORM FEED(codepoint 12, U + 000C)
			c == 0xd;		// CARRIAGE RETURN(codepoint 13, U + 000D)

		EXPECT_TRUE(strUtil::IsWhitespace(c) == is_White);
		EXPECT_TRUE(strUtil::IsWhitespaceW(static_cast<wchar_t>(c)) == is_White);
	}
}

TEST(StringUtil, IsAlphaNum) 
{
	// check every char.
	const char min = std::numeric_limits<char>::lowest();
	const char max = std::numeric_limits<char>::max();

	const int a = 'a';
	const int z = 'z';
	const int A = 'A';
	const int Z = 'Z';

	const int num0 = '0';
	const int num9 = '9';

	for (int c = min; c <= max; ++c)
	{
		bool is_alpha_low = c >= a && c <= z;
		bool is_alpha_high = c >= A && c <= Z;
		bool is_num = c >= num0 && c <= num9;

		bool is_alpha_num = is_alpha_low || is_alpha_high || is_num;

		EXPECT_TRUE(strUtil::IsWhitespace(static_cast<char>(c)) == is_alpha_num);
	}
}

TEST(StringUtil, IsAlphaNumU)
{
	// check every char.
	const char min = std::numeric_limits<uint8_t>::lowest();
	const char max = std::numeric_limits<uint8_t>::max();

	const int a = 'a';
	const int z = 'z';
	const int A = 'A';
	const int Z = 'Z';

	const int num0 = '0';
	const int num9 = '9';

	for (int c = min; c <= max; ++c)
	{
		bool is_alpha_low = c >= a && c <= z;
		bool is_alpha_high = c >= A && c <= Z;
		bool is_num = c >= num0 && c <= num9;

		bool is_alpha_num = is_alpha_low || is_alpha_high || is_num;

		EXPECT_TRUE(strUtil::IsWhitespaceW(static_cast<uint8_t>(c)) == is_alpha_num);
	}
}

TEST(StringUtil, Digit) {

	// check every char.
	const char min = std::numeric_limits<char>::lowest();
	const char max = std::numeric_limits<char>::max();

	for (int c = min; c <= max; c++)
	{
		bool is_Digit = c >= '0' && c <= '9';
		EXPECT_TRUE(strUtil::IsDigit(c) == is_Digit);
	}
}

TEST(StringUtil, DigitW) {

	// check every char.
	const wchar_t min = std::numeric_limits<wchar_t>::lowest();
	const wchar_t max = std::numeric_limits<wchar_t>::max();

	for (int c = min; c <= max; c++)
	{
		bool is_Digit = c >= '0' && c <= '9';
		EXPECT_TRUE(strUtil::IsDigitW(static_cast<wchar_t>(c)) == is_Digit);
	}
}

TEST(StringUtil, Numeric) {

	EXPECT_TRUE(strUtil::IsNumeric("52662374"));
	EXPECT_TRUE(strUtil::IsNumeric("2"));
	EXPECT_TRUE(strUtil::IsNumeric("52674"));

	EXPECT_FALSE(strUtil::IsNumeric("potato"));
	EXPECT_FALSE(strUtil::IsNumeric("camel"));

}

TEST(StringUtil, StrLen) {

	EXPECT_TRUE(strUtil::strlen("6  dig") == 6);
	EXPECT_TRUE(strUtil::strlen("hello chiken man") == 16);
	EXPECT_TRUE(strUtil::strlen("meow meow meow!") == 15);
	EXPECT_TRUE(strUtil::strlen("potato") == 6);

}

TEST(StringUtil, StrLenW) {

	EXPECT_TRUE(strUtil::strlen(L"6  dig") == 6);
	EXPECT_TRUE(strUtil::strlen(L"hello chiken man") == 16);
	EXPECT_TRUE(strUtil::strlen(L"meow meow meow!") == 15);
	EXPECT_TRUE(strUtil::strlen(L"potato") == 6);

}

TEST(StringUtil, Wide) {

	const wchar_t* pWide = L"wideee like your ass o.o";

	char Out[128] = {};
	char OutSub[128] = {};

	const char* full = strUtil::Convert(pWide, Out);
	const char* sub = strUtil::Convert(pWide, OutSub, 7);

	EXPECT_TRUE(strcmp("wideee like your ass o.o", full) == 0);
	EXPECT_TRUE(strcmp("wideee", OutSub) == 0);
}

TEST(StringUtil, Count) {

	StackString512 str("my name is bob. Hello jane.");

	int e_num = strUtil::Count(str.begin(), str.end(), 'e');
	int o_num = strUtil::Count(str.begin(), str.end(), 'o');

	EXPECT_TRUE(e_num == 3);
	EXPECT_TRUE(o_num == 2);
}

TEST(StringUtil, Equal) {

	StackString512 str("Hello jane.");
	StackString512 str1("Hello jane.");
	StackString512 upper("HELLO JaNe.");

	bool equal_1 = strUtil::IsEqual(str.begin(), str.end(), str1.begin(), str1.end());
	bool equal_2 = strUtil::IsEqual(str.begin(), str.end(), str1.c_str());
	bool equal_3 = strUtil::IsEqual(str.begin(), str1.c_str());
	bool equal_fail = strUtil::IsEqual(str.begin(), upper.c_str());

	bool equal_CI = strUtil::IsEqualCaseInsen(str.begin(), upper.c_str());

	EXPECT_TRUE(equal_1);
	EXPECT_TRUE(equal_2);
	EXPECT_TRUE(equal_3);
	EXPECT_TRUE(equal_CI);
	EXPECT_FALSE(equal_fail);
}


TEST(StringUtil, Int) {

	EXPECT_TRUE(strUtil::StringToInt<int>("1") == 1);
	EXPECT_TRUE(strUtil::StringToInt<int>("+1") == 1);
	EXPECT_TRUE(strUtil::StringToInt<int>("-1") == -1);
	EXPECT_TRUE(strUtil::StringToInt<int>("+-1") == 0);
	EXPECT_TRUE(strUtil::StringToInt<int>("-+1") == 0);

	EXPECT_TRUE(strUtil::StringToInt<int>("1.0") == 1);
	EXPECT_TRUE(strUtil::StringToInt<int>("+1.0") == 1);
	EXPECT_TRUE(strUtil::StringToInt<int>("-1.0") == -1);
	EXPECT_TRUE(strUtil::StringToInt<int>("+-1.0") == 0);
	EXPECT_TRUE(strUtil::StringToInt<int>("-+1.0") == 0);
}


TEST(StringUtil, Float) {

	EXPECT_TRUE(strUtil::StringToFloat<int>("1") == 1.f);
	EXPECT_TRUE(strUtil::StringToFloat<int>("+1") == 1.f);
	EXPECT_TRUE(strUtil::StringToFloat<int>("-1") == -1.f);
	EXPECT_TRUE(strUtil::StringToFloat<int>("+-1") == 0.f);
	EXPECT_TRUE(strUtil::StringToFloat<int>("-+1") == 0.f);

	EXPECT_TRUE(strUtil::StringToFloat<int>("1.0") == 1.f);
	EXPECT_TRUE(strUtil::StringToFloat<int>("+1.0") == 1.f);
	EXPECT_TRUE(strUtil::StringToFloat<int>("-1.0") == -1.f);
	EXPECT_TRUE(strUtil::StringToFloat<int>("+-1.0") == 0.f);
	EXPECT_TRUE(strUtil::StringToFloat<int>("-+1.0") == 0.f);

}


TEST(StringUtil, Hash) {

	StrHash hash("default.cfg");
	StrHash file("c_model_kinky_normal.tex");

	EXPECT_TRUE(hash == 0xd06dd0ee);
	EXPECT_TRUE(file == 0x22f7acea);
}