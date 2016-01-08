#include "stdafx.h"


#include <String\StackString.h>

X_USING_NAMESPACE;


TEST(StackStringW, Construct)
{
	core::StackString<32,wchar_t> str32_empty;
	core::StackString<32,wchar_t> str32_str(L"hello bob");
	core::StackString<32,wchar_t> str32_bool(true);
	core::StackString<32,wchar_t> str32_char('a');
	core::StackString<32,wchar_t> str32_int(-101);
	core::StackString<32,wchar_t> str32_uint(101u);
	core::StackString<32,wchar_t> str32_float(0.404054f);
	core::StackString<32,wchar_t> str32_64(123456789123456454);
	core::StackString<32,wchar_t> str32_u64(123456789123456454ull);
	core::StackString<32,wchar_t> str32_startEnd(str32_str.begin(), str32_str.end());

	core::StringRange<wchar_t> range(str32_str.begin(), str32_str.end());

	core::StackString<32,wchar_t> str32_range(range);


	EXPECT_STREQ(L"", str32_empty.c_str());
	EXPECT_STREQ(L"hello bob", str32_str.c_str());
	EXPECT_STREQ(L"1", str32_bool.c_str());
	EXPECT_STREQ(L"a", str32_char.c_str());
	EXPECT_STREQ(L"-101", str32_int.c_str());
	EXPECT_STREQ(L"101", str32_uint.c_str());
	EXPECT_STREQ(L"0.404054", str32_float.c_str());
	EXPECT_STREQ(L"123456789123456454", str32_64.c_str());
	EXPECT_STREQ(L"123456789123456454", str32_u64.c_str());
	EXPECT_STREQ(L"hello bob", str32_startEnd.c_str());
	EXPECT_STREQ(L"hello bob", str32_range.c_str());
}


TEST(StackStringW, Set)
{
	core::StackString<1024, wchar_t> str(L"goat man");

	str.set(L"Lorem Ipsum is simply dummy text of the printing and"
		L" typesetting industry.Lorem Ipsum has been the industry's"
		L" standard dummy text ever since the 1500");

	EXPECT_STREQ(L"Lorem Ipsum is simply dummy text of the printing and"
		L" typesetting industry.Lorem Ipsum has been the industry's"
		L" standard dummy text ever since the 1500", str.c_str());

	const wchar_t testTxt[38] = { L"slap a goat in a moat while in a boat" };

	str.set(testTxt, testTxt + 38);

	EXPECT_STREQ(testTxt, str.c_str());
}


TEST(StackStringW, Append)
{
	core::StackString<1024, wchar_t> str(L"camel man ");

	str.append(L'a', 5);
	EXPECT_STREQ(L"camel man aaaaa", str.c_str());

	str.append(L" goat");
	EXPECT_STREQ(L"camel man aaaaa goat", str.c_str());

	str.append(L" cat", 3);
	EXPECT_STREQ(L"camel man aaaaa goat ca", str.c_str());

	const wchar_t testTxt[10] = { L"hookersss" };
	str.append(testTxt, testTxt + 10);
	EXPECT_STREQ(L"camel man aaaaa goat cahookersss", str.c_str());
}

TEST(StackStringW, AppendFmt)
{
	core::StackString<1024, wchar_t> str(L" ");

	str.appendFmt(L"%i%s_%u", 1337, L"shit", 0x4096);

	EXPECT_STREQ(L" 1337shit_16534", str.c_str());

}

TEST(StackStringW, Replace)
{
	core::StackString<1024, wchar_t> str(L"tickle my pickle, i love a good tickle. my my");

	EXPECT_EQ(45, str.length());

	str.replace(L"tickle", L"touch");
	EXPECT_STREQ(L"touch my pickle, i love a good tickle. my my", str.c_str());
	EXPECT_EQ(44, str.length());

	str.replace(str.begin() + 15, L"my", L"goat");
	EXPECT_STREQ(L"touch my pickle, i love a good tickle. goat my", str.c_str());

	str.replace(L'o', L'x');
	EXPECT_STREQ(L"txuch my pickle, i love a good tickle. goat my", str.c_str());

	str.replaceAll(L'o', L'x');
	EXPECT_STREQ(L"txuch my pickle, i lxve a gxxd tickle. gxat my", str.c_str());

	str.replaceAll(L"my", L"pie");
	EXPECT_STREQ(L"txuch pie pickle, i lxve a gxxd tickle. gxat pie", str.c_str());

}

TEST(StackStringW, TrimWhiteSpace)
{
	core::StackString<1024,wchar_t> str(L"   show me your whitespace    ");

	str.trimWhitespace();
	EXPECT_STREQ(L"show me your whitespace", str.c_str());
}

TEST(StackStringW, TrimWhiteSpaceLeft)
{
	core::StackString<1024,wchar_t> str(L"    show me your whitespace    ");

	core::StackString<1024,wchar_t> str1 = str.trimLeft();
	EXPECT_STREQ(L"show me your whitespace    ", str.c_str());
	EXPECT_STREQ(L"show me your whitespace    ", str1.c_str());
}

TEST(StackStringW, TrimWhiteSpaceRight)
{
	core::StackString<1024,wchar_t> str(L"    show me your whitespace    ");

	core::StackString<1024,wchar_t> str1 = str.trimRight();
	EXPECT_STREQ(L"    show me your whitespace", str.c_str());
	EXPECT_STREQ(L"    show me your whitespace", str1.c_str());
}

TEST(StackStringW, TrimWhiteSpaceLeftRight)
{
	core::StackString<1024,wchar_t> str(L"    show me your whitespace    ");

	core::StackString<1024,wchar_t> str1 = str.trim();
	EXPECT_STREQ(L"show me your whitespace", str.c_str());
	EXPECT_STREQ(L"show me your whitespace", str1.c_str());
}

TEST(StackStringW, trimCharacter)
{
	core::StackString<1024,wchar_t> str(L"--- -----show me your whitespace------");

	str.trimCharacter(L'-');
	EXPECT_STREQ(L" -----show me your whitespace", str.c_str());
}

TEST(StackStringW, trimRight)
{
	core::StackString<1024,wchar_t> str(L"--- -----show me your whitespace------");

	str.trimRight(L'w');
	EXPECT_STREQ(L"--- -----sho", str.c_str());

	str.trimRight(str.c_str() + 10);
	EXPECT_STREQ(L"--- -----s", str.c_str());
}


TEST(StackStringW, stripTrailing)
{
	core::StackString<1024,wchar_t> str(L"--- -----show me your whitespace------");

	str.stripTrailing(L'e');
	EXPECT_STREQ(L"--- -----show me your whitespace------", str.c_str());

	str.stripTrailing(L'-');
	EXPECT_STREQ(L"--- -----show me your whitespace", str.c_str());
}


TEST(StackStringW, Clear)
{
	core::StackString<1024,wchar_t> str(L"When the goat is in the moat it's blocked");

	str.clear();

	EXPECT_EQ(0, str.length());
	EXPECT_EQ(1024, str.capacity());
}


TEST(StackStringW, isEqual)
{
	core::StackString<64,wchar_t> str(L"goat");
	core::StackString<64,wchar_t> str1(L"goAt");
	core::StackString<64,wchar_t> str2(L"goat");

	EXPECT_FALSE(str.isEqual(str1.c_str()));
	EXPECT_FALSE(str.isEqual(L"gOat"));

	EXPECT_TRUE(str.isEqual(str2.c_str()));
	EXPECT_TRUE(str.isEqual(L"goat"));
}


TEST(StackStringW, findLast)
{
	core::StackString<128,wchar_t> str(L"ooogoogodoghdooogdgoooat");

	const wchar_t* pFind = str.findLast(L'o');
	EXPECT_TRUE(pFind != nullptr);
	EXPECT_TRUE(pFind == (str.end() - 3));

	EXPECT_TRUE(str.findLast(L'x') == nullptr);
}

TEST(StackStringW, find)
{
	core::StackString<128,wchar_t> str(L"ooogoogodoghdooogdgoooat");

	const wchar_t* pFind = str.find(L'g');
	EXPECT_TRUE(pFind != nullptr);
	EXPECT_TRUE(pFind == (str.begin() + 3));

	EXPECT_TRUE(str.find(L'x') == nullptr);

	const wchar_t* pFind2 = str.find(L"gooo");
	EXPECT_TRUE(pFind2 != nullptr);
	EXPECT_TRUE(pFind2 == (str.begin() + 18));
}

TEST(StackStringW, findCaseInsen)
{
	core::StackString<128,wchar_t> str(L"ooogoogodoghdXooogdGoooat");

	const wchar_t* pFind = str.findCaseInsen(L'g');
	EXPECT_TRUE(pFind != nullptr);
	EXPECT_TRUE(pFind == (str.begin() + 3));

	EXPECT_TRUE(str.findCaseInsen(L'x') != nullptr);
	EXPECT_TRUE(str.findCaseInsen(L'X') != nullptr);
	EXPECT_TRUE(str.findCaseInsen(L'y') == nullptr);
	EXPECT_TRUE(str.findCaseInsen(L'Y') == nullptr);

	const wchar_t* pFind2 = str.findCaseInsen(L"Gooo");
	EXPECT_TRUE(pFind2 != nullptr);
	EXPECT_TRUE(pFind2 == (str.begin() + 19));
}

TEST(StackStringW, operatorEqual)
{
	core::StackString<64,wchar_t> str(L"goat");
	core::StackString<64,wchar_t> str1(L"goat");
	core::StackString<64,wchar_t> str2(L"goaT");

	EXPECT_TRUE(str == str1);
	EXPECT_FALSE(str == str2);
}

TEST(StackStringW, operatorNotEqual)
{
	core::StackString<64,wchar_t> str(L"goat");
	core::StackString<64,wchar_t> str1(L"goat");
	core::StackString<64,wchar_t> str2(L"goaT");

	EXPECT_FALSE(str != str1);
	EXPECT_TRUE(str != str2);
}

TEST(StackStringW, operatorAssign)
{
	core::StackString<64,wchar_t> str(L"goat");
	core::StackString<64,wchar_t> str2(L"goaT");

	str = str2;

	EXPECT_STREQ(L"goaT", str.c_str());
}

TEST(StackStringW, operatorArray)
{
	core::StackString<64,wchar_t> str(L"goat");

	EXPECT_EQ(L'g', str[0]);
	EXPECT_EQ(L'o', str[1]);
	EXPECT_EQ(L'a', str[2]);
	EXPECT_EQ(L't', str[3]);

	// i allow access to null term.
	EXPECT_EQ(L'\0', str[4]);

	core::debugging::EnableBreakpoints(false);
	g_AssetChecker.ExpectAssertion(true);

	// out of range.
	wchar_t val = str[5];

	EXPECT_TRUE(g_AssetChecker.HadCorrectAssertions());

	g_AssetChecker.ExpectAssertion(false);
	core::debugging::EnableBreakpoints(true);
}

TEST(StackStringW, c_str)
{
	core::StackString<64,wchar_t> str(L"goat");
	core::StackString<64,wchar_t> str1(L"");
	core::StackString<32,wchar_t> str2(L"thistring is too long for me ee");

	EXPECT_STREQ(L"goat", str.c_str());
	EXPECT_STREQ(L"", str1.c_str());
	EXPECT_STREQ(L"thistring is too long for me ee", str2.c_str());
}


TEST(StackStringW, length)
{
	core::StackString<64,wchar_t> str(L"goat");
	core::StackString<64,wchar_t> str1(L"");
	core::StackString<32,wchar_t> str2(L"thistring is too long for me ee");

	EXPECT_EQ(4, str.length());
	EXPECT_EQ(0, str1.length());
	EXPECT_EQ(31, str2.length());
}

TEST(StackStringW, capacity)
{
	core::StackString<64,wchar_t> str(L"goat");
	core::StackString<64,wchar_t> str1(L"");
	core::StackString<32,wchar_t> str2(L"thistring is too long for me ee");

	EXPECT_EQ(64, str.capacity());
	EXPECT_EQ(64, str1.capacity());
	EXPECT_EQ(32, str2.capacity());
}

TEST(StackStringW, isEmpty)
{
	core::StackString<64,wchar_t> str(L"goat");
	core::StackString<64,wchar_t> str1(L"");

	EXPECT_FALSE(str.isEmpty());
	EXPECT_TRUE(str1.isEmpty());

	str.clear();

	EXPECT_TRUE(str.isEmpty());
}

TEST(StackStringW, isNotEmpty)
{
	core::StackString<64,wchar_t> str(L"goat");
	core::StackString<64,wchar_t> str1(L"");

	EXPECT_TRUE(str.isNotEmpty());
	EXPECT_FALSE(str1.isNotEmpty());

	str.clear();

	EXPECT_FALSE(str.isNotEmpty());
}

TEST(StackStringW, toLower)
{
	core::StackString<64,wchar_t> str(L"goat");
	core::StackString<64,wchar_t> str1(L"GOATMAN");
	core::StackString<64,wchar_t> str2(L"GoaTMaN");

	EXPECT_STREQ(L"GoaTMaN", str2.c_str());

	str.toLower();
	str1.toLower();
	str2.toLower();

	EXPECT_STREQ(L"goat", str.c_str());
	EXPECT_STREQ(L"goatman", str1.c_str());
	EXPECT_STREQ(L"goatman", str2.c_str());
}

TEST(StackStringW, toUpper)
{
	core::StackString<64,wchar_t> str(L"goat");
	core::StackString<64,wchar_t> str1(L"goatMan");
	core::StackString<64,wchar_t> str2(L"GoaTMaN");

	EXPECT_STREQ(L"GoaTMaN", str2.c_str());

	str.toUpper();
	str1.toUpper();
	str2.toUpper();

	EXPECT_STREQ(L"GOAT", str.c_str());
	EXPECT_STREQ(L"GOATMAN", str1.c_str());
	EXPECT_STREQ(L"GOATMAN", str2.c_str());
}


TEST(StackStringW, begin)
{
	core::StackString<64,wchar_t> str(L"goat");
	core::StackString<64,wchar_t> str1(L"goatMan");
	core::StackString<64,wchar_t> str2(L"GoaTMaN");

	EXPECT_STREQ(L"goat", str.begin());
	EXPECT_STREQ(L"goatMan", str1.begin());
	EXPECT_STREQ(L"GoaTMaN", str2.begin());
}

TEST(StackStringW, end)
{
	core::StackString<64,wchar_t> str(L"goat");
	core::StackString<64,wchar_t> str1(L"goatMan");
	core::StackString<64,wchar_t> str2(L"GoaTMaN");

	EXPECT_EQ(str.begin() + 4, str.end());
	EXPECT_EQ(str1.begin() + 7, str1.end());
	EXPECT_EQ(str2.begin() + 7, str2.end());
}