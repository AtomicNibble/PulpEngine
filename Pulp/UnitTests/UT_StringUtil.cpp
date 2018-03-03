#include "stdafx.h"



#include <String\StringHash.h>


X_USING_NAMESPACE;

using namespace core;


TEST(StringUtil, StringBytes) {

	std::string str = "how long is this txt?";
	std::wstring strW = L"how long is this txt?";
	core::string cstr = core::string("how long is this txt?"); 
	core::StringRef<wchar_t> cstrW = core::StringRef<wchar_t>(L"how long is this txt?");

	EXPECT_EQ(21, strUtil::StringBytes(str));
	EXPECT_EQ(21 * 2, strUtil::StringBytes(strW));
	EXPECT_EQ(21, strUtil::StringBytes(cstr));
	EXPECT_EQ(21 * 2, strUtil::StringBytes(cstrW));
}

TEST(StringUtil, StringBytesNUll) {

	std::string str = "how long is this txt?";
	std::wstring strW = L"how long is this txt?";
	core::string cstr = core::string("how long is this txt?");
	core::StringRef<wchar_t> cstrW = core::StringRef<wchar_t>(L"how long is this txt?");

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
	const char* fin_fail2 = strUtil::FindCaseInsensitive(str2.begin(), str2.begin() + 10, "bob");
	
	EXPECT_TRUE(find == (str.begin() + 16));
	EXPECT_FALSE(findNCI == (str.begin() + 16)); // should not find as case is diffrent
	EXPECT_TRUE(findCI == (str.begin() + 16));
	EXPECT_TRUE(fin_fail == nullptr); // should not find as case is diffrent
	EXPECT_TRUE(fin_fail2 == nullptr); // should not find as end is before null term
}

TEST(StringUtil, FindW) {

	StackStringW512 str(L"my name is bob. Hello jane.");
	StackStringW512 str2(L"Hell");

	const wchar_t* find = strUtil::Find(str.begin(), str.end(), L"Hello");
	const wchar_t* findNCI = strUtil::Find(str.begin(), str.end(), L"hello");
	const wchar_t* findCI = strUtil::FindCaseInsensitive(str.begin(), str.end(), L"hello");
	const wchar_t* fin_fail = strUtil::FindCaseInsensitive(str2.begin(), str2.end(), L"hello");
	const wchar_t* fin_fail2 = strUtil::FindCaseInsensitive(str2.begin(), str2.begin() + 10, L"bob");

	EXPECT_TRUE(find == (str.begin() + 16));
	EXPECT_FALSE(findNCI == (str.begin() + 16)); // should not find as case is diffrent
	EXPECT_TRUE(findCI == (str.begin() + 16));
	EXPECT_TRUE(fin_fail == nullptr); // should not find as case is diffrent
	EXPECT_TRUE(fin_fail2 == nullptr); // should not find as end is before null term
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

TEST(StringUtil, FindW2)
{
	// had a bug where find was returning true after only matching 2 chars.
	// this test checks that is fixed.
	StackStringW512 str(L"my Hell name is bob. Hello jane.");

	const wchar_t* find = strUtil::Find(str.begin(), str.end(), L"Hello");
	const wchar_t* findCI = strUtil::FindCaseInsensitive(str.begin(), str.end(), L"hello");

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

		EXPECT_TRUE(strUtil::IsAlphaNum(static_cast<char>(c)) == is_alpha_num);
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

		EXPECT_TRUE(strUtil::IsAlphaNum(static_cast<uint8_t>(c)) == is_alpha_num);
	}
}

TEST(StringUtil, IsAlpha)
{
	// check every char.
	const char min = std::numeric_limits<char>::lowest();
	const char max = std::numeric_limits<char>::max();

	const int a = 'a';
	const int z = 'z';
	const int A = 'A';
	const int Z = 'Z';

	for (int c = min; c <= max; ++c)
	{
		bool is_alpha_low = c >= a && c <= z;
		bool is_alpha_high = c >= A && c <= Z;

		bool is_alpha = is_alpha_low || is_alpha_high;

		EXPECT_TRUE(strUtil::IsAlpha(static_cast<char>(c)) == is_alpha);
	}
}

TEST(StringUtil, IsAlphaU)
{
	// check every char.
	const char min = std::numeric_limits<uint8_t>::lowest();
	const char max = std::numeric_limits<uint8_t>::max();

	const int a = 'a';
	const int z = 'z';
	const int A = 'A';
	const int Z = 'Z';

	for (int c = min; c <= max; ++c)
	{
		bool is_alpha_low = c >= a && c <= z;
		bool is_alpha_high = c >= A && c <= Z;

		bool is_alpha = is_alpha_low || is_alpha_high;

		EXPECT_TRUE(strUtil::IsAlpha(static_cast<uint8_t>(c)) == is_alpha);
	}
}

TEST(StringUtil, IsLower)
{
	EXPECT_TRUE(core::strUtil::IsLower('c'));
	EXPECT_TRUE(core::strUtil::IsLower('a'));
	EXPECT_FALSE(core::strUtil::IsLower('C'));
	EXPECT_FALSE(core::strUtil::IsLower('G'));

	EXPECT_TRUE(core::strUtil::IsLower("tickle my pickle 342 'sfs'f1414"));
	EXPECT_FALSE(core::strUtil::IsLower("tickle mN pickle 342 'sfs'f1414"));

	char testStr[] = ";gsgsdlpwotimbnaw536sv0'252'v'302,v";
	EXPECT_TRUE(core::strUtil::IsLower("tickle my pickle 342 'sfs'f1414"));
	EXPECT_TRUE(core::strUtil::IsLower(testStr, testStr + sizeof(testStr)));

	char testStr2[] = "hello i'm a GOAT";
	EXPECT_TRUE(core::strUtil::IsLower(testStr2, testStr2 + (sizeof(testStr2) - 5)));
	EXPECT_FALSE(core::strUtil::IsLower(testStr2, testStr2 + sizeof(testStr2)));

	EXPECT_TRUE(core::strUtil::IsLower(""));
}

TEST(StringUtil, IsLowerW)
{
	EXPECT_TRUE(core::strUtil::IsLowerW(L'c'));
	EXPECT_TRUE(core::strUtil::IsLowerW(L'a'));
	EXPECT_FALSE(core::strUtil::IsLowerW(L'C'));
	EXPECT_FALSE(core::strUtil::IsLowerW(L'G'));


	EXPECT_TRUE(core::strUtil::IsLower(L"tickle my pickle 342 'sfs'f1414"));
	EXPECT_FALSE(core::strUtil::IsLower(L"tickle mN pickle 342 'sfs'f1414"));

	wchar_t testStr[] = L";gsgsdlpwotimbnaw536sv0'252'v'302,v";
	EXPECT_TRUE(core::strUtil::IsLower(L"tickle my pickle 342 'sfs'f1414"));
	EXPECT_TRUE(core::strUtil::IsLower(testStr, testStr + (sizeof(testStr) / 2)));

	wchar_t testStr2[] = L"hello i'm a GOAT";
	EXPECT_TRUE(core::strUtil::IsLower(testStr2, testStr2 + ((sizeof(testStr2)/2) - 5)));
	EXPECT_FALSE(core::strUtil::IsLower(testStr2, testStr2 + (sizeof(testStr2) / 2)));

	EXPECT_TRUE(core::strUtil::IsLower(""));
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
	StackString512 str_longer("Hello jane. meow");
	StackString512 str1("Hello jane.");
	StackString512 upper("HELLO JaNe.");

	bool equal_1 = strUtil::IsEqual(str.begin(), str.end(), str1.begin(), str1.end());
	bool equal_2 = strUtil::IsEqual(str.begin(), str.end(), str1.c_str());
	bool equal_3 = strUtil::IsEqual(str.begin(), str1.c_str());
	bool equal_fail = strUtil::IsEqual(str.begin(), upper.c_str());

	// should not match now, due to outcome of issue #4
	bool equal_fail2 = strUtil::IsEqual(str.begin(), str.end(), str_longer.c_str());


	EXPECT_TRUE(equal_1);
	EXPECT_TRUE(equal_2);
	EXPECT_TRUE(equal_3);
	EXPECT_FALSE(equal_fail);
	EXPECT_FALSE(equal_fail2);
}

TEST(StringUtil, EqualW) {

	StackStringW512 str(L"Hello jane.");
	StackStringW512 str_longer(L"Hello jane. meow");
	StackStringW512 str1(L"Hello jane.");
	StackStringW512 upper(L"HELLO JaNe.");

	bool equal_1 = strUtil::IsEqual(str.begin(), str.end(), str1.begin(), str1.end());
	bool equal_2 = strUtil::IsEqual(str.begin(), str.end(), str1.c_str());
	bool equal_3 = strUtil::IsEqual(str.begin(), str1.c_str());
	bool equal_fail = strUtil::IsEqual(str.begin(), upper.c_str());

	// should not match now, due to outcome of issue #4
	bool equal_fail2 = strUtil::IsEqual(str.begin(), str.end(), str_longer.c_str());

	EXPECT_TRUE(equal_1);
	EXPECT_TRUE(equal_2);
	EXPECT_TRUE(equal_3);
	EXPECT_FALSE(equal_fail);
	EXPECT_FALSE(equal_fail2);
}

TEST(StringUtil, EqualCaseInsen) {

	StackString512 str("Hello jane.");
	StackString512 str1("Hello jane");
	StackString512 str2("Hello jane,");
	StackString512 upper("HELLO JaNe.");

	bool equal_1_true = strUtil::IsEqualCaseInsen(str.begin(), upper.c_str());
	bool equal_2_false = strUtil::IsEqualCaseInsen(str1.begin(), str1.end(), str2.c_str());
	bool equal_3_true = strUtil::IsEqualCaseInsen(str.begin(), str.end(), upper.c_str());
	bool equal_4_true = strUtil::IsEqualCaseInsen(str.begin(), str.end(), upper.c_str(), upper.end());
	bool equal_5_false = strUtil::IsEqualCaseInsen(str.begin(), str.end(), str2.c_str(), str2.end());
	bool equal_6_true = strUtil::IsEqualCaseInsen(str.begin(), str.end() - 1, str2.c_str(), str2.end() - 1);
	bool equal_7_true = strUtil::IsEqualCaseInsen(str.begin(), str.end() - 1, str1.c_str());

	EXPECT_TRUE(equal_1_true);
	EXPECT_FALSE(equal_2_false);
	EXPECT_TRUE(equal_3_true);
	EXPECT_TRUE(equal_4_true);
	EXPECT_FALSE(equal_5_false);
	EXPECT_TRUE(equal_6_true);
	EXPECT_TRUE(equal_7_true);
}

TEST(StringUtil, EqualCaseInsenW) {

	StackStringW512 str(L"Hello jane.");
	StackStringW512 str1(L"Hello jane");
	StackStringW512 str2(L"Hello jane,");
	StackStringW512 upper(L"HELLO JaNe.");

	bool equal_1_true = strUtil::IsEqualCaseInsen(str.begin(), upper.c_str());
	bool equal_2_false = strUtil::IsEqualCaseInsen(str1.begin(), str1.end(), str2.c_str());
	bool equal_3_true = strUtil::IsEqualCaseInsen(str.begin(), str.end(), upper.c_str());
	bool equal_4_true = strUtil::IsEqualCaseInsen(str.begin(), str.end(), upper.c_str(), upper.end());
	bool equal_5_false = strUtil::IsEqualCaseInsen(str.begin(), str.end(), str2.c_str(), str2.end());
	bool equal_6_true = strUtil::IsEqualCaseInsen(str.begin(), str.end() - 1, str2.c_str(), str2.end() - 1);
	bool equal_7_true = strUtil::IsEqualCaseInsen(str.begin(), str.end() - 1, str1.c_str());

	EXPECT_TRUE(equal_1_true);
	EXPECT_FALSE(equal_2_false);
	EXPECT_TRUE(equal_3_true);
	EXPECT_TRUE(equal_4_true);
	EXPECT_FALSE(equal_5_false);
	EXPECT_TRUE(equal_6_true);
	EXPECT_TRUE(equal_7_true);
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

	// range
	const char* testStr0 = " 17959";
	const char* testStr1 = " +16363 ";
	const char* testStr2 = "-1 25";
	const char* testStr3 = "+-1 ";
	const char* testStr4 = "-+1 ";

	EXPECT_EQ(1, strUtil::StringToInt<int>(testStr0, testStr0 + 2));
	EXPECT_EQ(1, strUtil::StringToInt<int>(testStr1, testStr1 + 3));
	EXPECT_EQ(-1, strUtil::StringToInt<int>(testStr2, testStr2 + 2));
	EXPECT_EQ(0, strUtil::StringToInt<int>(testStr3, testStr3 + 3));
	EXPECT_EQ(0, strUtil::StringToInt<int>(testStr4, testStr4 + 3));

}


TEST(StringUtil, IntW) {

	EXPECT_TRUE(strUtil::StringToInt<int>(L"1") == 1);
	EXPECT_TRUE(strUtil::StringToInt<int>(L"+1") == 1);
	EXPECT_TRUE(strUtil::StringToInt<int>(L"-1") == -1);
	EXPECT_TRUE(strUtil::StringToInt<int>(L"+-1") == 0);
	EXPECT_TRUE(strUtil::StringToInt<int>(L"-+1") == 0);

	EXPECT_TRUE(strUtil::StringToInt<int>(L"1.0") == 1);
	EXPECT_TRUE(strUtil::StringToInt<int>(L"+1.0") == 1);
	EXPECT_TRUE(strUtil::StringToInt<int>(L"-1.0") == -1);
	EXPECT_TRUE(strUtil::StringToInt<int>(L"+-1.0") == 0);
	EXPECT_TRUE(strUtil::StringToInt<int>(L"-+1.0") == 0);

	// range
	const wchar_t* testStr0 = L" 17959";
	const wchar_t* testStr1 = L" +16363 ";
	const wchar_t* testStr2 = L"-1 25";
	const wchar_t* testStr3 = L"+-1 ";
	const wchar_t* testStr4 = L"-+1 ";

	EXPECT_EQ(1, strUtil::StringToInt<int>(testStr0, testStr0 + 2));
	EXPECT_EQ(1, strUtil::StringToInt<int>(testStr1, testStr1 + 3));
	EXPECT_EQ(-1, strUtil::StringToInt<int>(testStr2, testStr2 + 2));
	EXPECT_EQ(0, strUtil::StringToInt<int>(testStr3, testStr3 + 3));
	EXPECT_EQ(0, strUtil::StringToInt<int>(testStr4, testStr4 + 3));
}


TEST(StringUtil, Float) {

	EXPECT_FLOAT_EQ(strUtil::StringToFloat<float>("1"), 1.f);
	EXPECT_FLOAT_EQ(strUtil::StringToFloat<float>("+1"), 1.f);
	EXPECT_FLOAT_EQ(strUtil::StringToFloat<float>("-1"), -1.f);
	EXPECT_FLOAT_EQ(strUtil::StringToFloat<float>("+-1"), 0.f);
	EXPECT_FLOAT_EQ(strUtil::StringToFloat<float>("-+1"), 0.f);

	EXPECT_FLOAT_EQ(strUtil::StringToFloat<float>("1.0"), 1.f);
	EXPECT_FLOAT_EQ(strUtil::StringToFloat<float>("+1.0"), 1.f);
	EXPECT_FLOAT_EQ(strUtil::StringToFloat<float>("-1.0"), -1.f);
	EXPECT_FLOAT_EQ(strUtil::StringToFloat<float>("+-1.0"), 0.f);
	EXPECT_FLOAT_EQ(strUtil::StringToFloat<float>("-+1.0"), 0.f);

}


TEST(StringUtil, FloatW) {

	EXPECT_FLOAT_EQ(strUtil::StringToFloat<float>(L"1"), 1.f);
	EXPECT_FLOAT_EQ(strUtil::StringToFloat<float>(L"+1"), 1.f);
	EXPECT_FLOAT_EQ(strUtil::StringToFloat<float>(L"-1"), -1.f);
	EXPECT_FLOAT_EQ(strUtil::StringToFloat<float>(L"+-1"), 0.f);
	EXPECT_FLOAT_EQ(strUtil::StringToFloat<float>(L"-+1"), 0.f);

	EXPECT_FLOAT_EQ(strUtil::StringToFloat<float>(L"1.0"), 1.f);
	EXPECT_FLOAT_EQ(strUtil::StringToFloat<float>(L"+1.0"), 1.f);
	EXPECT_FLOAT_EQ(strUtil::StringToFloat<float>(L"-1.0"), -1.f);
	EXPECT_FLOAT_EQ(strUtil::StringToFloat<float>(L"+-1.0"), 0.f);
	EXPECT_FLOAT_EQ(strUtil::StringToFloat<float>(L"-+1.0"), 0.f);

}

TEST(StringUtil, HasFileExtension)
{
	EXPECT_TRUE(strUtil::HasFileExtension("tickle_my_pickle~.h"));
	EXPECT_TRUE(strUtil::HasFileExtension("tickle_my.goat"));
	EXPECT_FALSE(strUtil::HasFileExtension("tickle_my_pickle~"));
	EXPECT_FALSE(strUtil::HasFileExtension("tickle_my_pic "));
	EXPECT_FALSE(strUtil::HasFileExtension("tickle_my_pic ."));

	StackString512 text("slap_a_carrot.goat");
	EXPECT_TRUE(strUtil::HasFileExtension(text.begin(), text.end()));
	EXPECT_TRUE(strUtil::HasFileExtension(text.begin(), text.end() - 2));
	EXPECT_TRUE(strUtil::HasFileExtension(text.begin() + 10, text.end() - 2));
	EXPECT_FALSE(strUtil::HasFileExtension(text.begin(), text.end() - 5));
}

TEST(StringUtil, HasFileExtensionW)
{
	EXPECT_TRUE(strUtil::HasFileExtension(L"tickle_my_pickle~.h"));
	EXPECT_TRUE(strUtil::HasFileExtension(L"tickle_my.goat"));
	EXPECT_FALSE(strUtil::HasFileExtension(L"tickle_my_pickle~"));
	EXPECT_FALSE(strUtil::HasFileExtension(L"tickle_my_pic "));
	EXPECT_FALSE(strUtil::HasFileExtension(L"tickle_my_pic ."));

	StackStringW512 text(L"slap_a_carrot.goat");
	EXPECT_TRUE(strUtil::HasFileExtension(text.begin(), text.end()));
	EXPECT_TRUE(strUtil::HasFileExtension(text.begin(), text.end() - 2));
	EXPECT_TRUE(strUtil::HasFileExtension(text.begin() + 10, text.end() - 2));
	EXPECT_FALSE(strUtil::HasFileExtension(text.begin(), text.end() - 5));
}

TEST(StringUtil, FileExtension)
{
	EXPECT_STREQ("h", strUtil::FileExtension("tickle_my_pickle~.h"));
	EXPECT_STREQ("chicken", strUtil::FileExtension("tickle_my_.chicken"));
	EXPECT_STREQ("dots", strUtil::FileExtension("how.many.dots"));
	EXPECT_EQ(nullptr, strUtil::FileExtension("tickle_my_pic "));
	EXPECT_EQ(nullptr,strUtil::FileExtension("tickle_my_pic ."));

	StackString512 text("slap_a_carrot.goat");

	EXPECT_STREQ("goat", strUtil::FileExtension(text.begin(), text.end()));
	EXPECT_EQ(nullptr, strUtil::FileExtension(text.begin(), text.end() - 4));

	StackString512 text1("slap_a_carrot.goat.cat");

	EXPECT_STREQ("cat", strUtil::FileExtension(text1.begin(), text1.end()));
	EXPECT_STREQ("goat.cat", strUtil::FileExtension(text1.begin(), text1.end() - 4));
	EXPECT_STREQ("goat.cat", strUtil::FileExtension(text1.begin() + 10, text1.end() - 4));
	EXPECT_EQ(nullptr, strUtil::FileExtension(text1.begin(), text1.end() - 8));
}


TEST(StringUtil, FileExtensionW)
{
	EXPECT_STREQ(L"h", strUtil::FileExtension(L"tickle_my_pickle~.h"));
	EXPECT_STREQ(L"chicken", strUtil::FileExtension(L"tickle_my_.chicken"));
	EXPECT_STREQ(L"dots", strUtil::FileExtension(L"how.many.dots"));
	EXPECT_EQ(nullptr, strUtil::FileExtension(L"tickle_my_pic "));
	EXPECT_EQ(nullptr, strUtil::FileExtension(L"tickle_my_pic ."));

	StackStringW512 text(L"slap_a_carrot.goat");

	EXPECT_STREQ(L"goat", strUtil::FileExtension(text.begin(), text.end()));
	EXPECT_EQ(nullptr, strUtil::FileExtension(text.begin(), text.end() - 4));

	StackStringW512 text1(L"slap_a_carrot.goat.cat");

	EXPECT_STREQ(L"cat", strUtil::FileExtension(text1.begin(), text1.end()));
	EXPECT_STREQ(L"goat.cat", strUtil::FileExtension(text1.begin(), text1.end() - 4));
	EXPECT_STREQ(L"goat.cat", strUtil::FileExtension(text1.begin() + 10, text1.end() - 4));
	EXPECT_EQ(nullptr, strUtil::FileExtension(text1.begin(), text1.end() - 8));
}

TEST(StringUtil, WildCard)
{
	EXPECT_TRUE(strUtil::WildCompare("*", "Hello"));
	EXPECT_TRUE(strUtil::WildCompare("**", "Hello"));
	EXPECT_TRUE(strUtil::WildCompare("*", "t"));
	EXPECT_TRUE(strUtil::WildCompare("***", "t"));
	EXPECT_TRUE(strUtil::WildCompare("*t", "_t"));
	EXPECT_TRUE(strUtil::WildCompare("*t*", "_t_"));
	EXPECT_TRUE(strUtil::WildCompare("***Example", "Example"));
	EXPECT_TRUE(strUtil::WildCompare("***Example", "space Example"));
	EXPECT_TRUE(strUtil::WildCompare("***Exa*mple", "space Exa_^$^+$^$+^$_mple"));
	EXPECT_TRUE(strUtil::WildCompare("Same", "Same"));
	// wild matches empty string since it's 0-N
	EXPECT_TRUE(strUtil::WildCompare("*", ""));

	EXPECT_FALSE(strUtil::WildCompare("Cow*CatDog*", "Cow Cat Dog"));
	EXPECT_FALSE(strUtil::WildCompare("LOWER", "lower"));
	EXPECT_FALSE(strUtil::WildCompare("y*NameIsTom", "MyNameIsTom"));

	// check with offsets
	// make some that would match without offset and offset.
//	EXPECT_FALSE(strUtil::WildCompare("Same", "Same", 1));
//	EXPECT_FALSE(strUtil::WildCompare("*Same*", "pre_Same_", 5));

	// only match if offset
//	EXPECT_TRUE(strUtil::WildCompare("Same", "offset_Same", 7));
//	EXPECT_TRUE(strUtil::WildCompare("Goat*", "pre_Goat_extra_chars", 4));
}

TEST(StringUtil, WildCardW)
{
	EXPECT_TRUE(strUtil::WildCompare(L"*", L"Hello"));
	EXPECT_TRUE(strUtil::WildCompare(L"**", L"Hello"));
	EXPECT_TRUE(strUtil::WildCompare(L"*", L"t"));
	EXPECT_TRUE(strUtil::WildCompare(L"***", L"t"));
	EXPECT_TRUE(strUtil::WildCompare(L"*t", L"_t"));
	EXPECT_TRUE(strUtil::WildCompare(L"*t*", L"_t_"));
	EXPECT_TRUE(strUtil::WildCompare(L"***Example", L"Example"));
	EXPECT_TRUE(strUtil::WildCompare(L"***Example", L"space Example"));
	EXPECT_TRUE(strUtil::WildCompare(L"***Exa*mple", L"space Exa_^$^+$^$+^$_mple"));
	EXPECT_TRUE(strUtil::WildCompare(L"Same", L"Same"));
	// wild matches empty string since it's 0-N
	EXPECT_TRUE(strUtil::WildCompare(L"*", L""));

	EXPECT_FALSE(strUtil::WildCompare(L"Cow*CatDog*", L"Cow Cat Dog"));
	EXPECT_FALSE(strUtil::WildCompare(L"LOWER", L"lower"));
	EXPECT_FALSE(strUtil::WildCompare(L"y*NameIsTom", L"MyNameIsTom"));

	// check with offsets
	// make some that would match without offset and offset.
	//	EXPECT_FALSE(strUtil::WildCompare("Same", "Same", 1));
	//	EXPECT_FALSE(strUtil::WildCompare("*Same*", "pre_Same_", 5));

	// only match if offset
	//	EXPECT_TRUE(strUtil::WildCompare("Same", "offset_Same", 7));
	//	EXPECT_TRUE(strUtil::WildCompare("Goat*", "pre_Goat_extra_chars", 4));
}


TEST(StringUtil, Hash) {

	StrHash hash("default.cfg");
	StrHash file("c_model_kinky_normal.tex");

	EXPECT_TRUE(hash == 0xd06dd0ee);
	EXPECT_TRUE(file == 0x22f7acea);
}