#include "stdafx.h"

#include <String\StringRange.h>

X_USING_NAMESPACE;

TEST(StringRange, Construct)
{
    char text[21] = "tickle my pickle plz";

    core::StringRange<char> range(nullptr, nullptr);
    core::StringRange<char> range1(text, text + strlen(text));
    core::StringRange<char> range2(text, text + strlen(text) / 2);
    core::StringRange<char> range3(text, strlen(text) - 1);

    EXPECT_EQ(0, range.getLength());
    EXPECT_EQ(20, range1.getLength());
    EXPECT_EQ(10, range2.getLength());
    EXPECT_EQ(19, range3.getLength());

    EXPECT_TRUE(core::strUtil::IsEqual(range1.getStart(), range1.getEnd(), text));
    EXPECT_TRUE(core::strUtil::IsEqual(range2.getStart(), range2.getEnd(), text, text + 10));
    EXPECT_TRUE(core::strUtil::IsEqual(range3.getStart(), range3.getEnd(), text, text + 19));
}

TEST(StringRange, ConstructW)
{
    wchar_t text[21] = L"tickle my pickle plz";

    core::StringRange<wchar_t> range(nullptr, nullptr);
    core::StringRange<wchar_t> range1(text, text + core::strUtil::strlen(text));
    core::StringRange<wchar_t> range2(text, text + core::strUtil::strlen(text) / 2);
    core::StringRange<wchar_t> range3(text, core::strUtil::strlen(text) - 1);

    EXPECT_EQ(0, range.getLength());
    EXPECT_EQ(20, range1.getLength());
    EXPECT_EQ(10, range2.getLength());
    EXPECT_EQ(19, range3.getLength());

    EXPECT_TRUE(core::strUtil::IsEqual(range1.getStart(), range1.getEnd(), text));
    EXPECT_TRUE(core::strUtil::IsEqual(range2.getStart(), range2.getEnd(), text, text + 10));
    EXPECT_TRUE(core::strUtil::IsEqual(range3.getStart(), range3.getEnd(), text, text + 19));
}

TEST(StringRange, Find)
{
    char text[21] = "tickle my pickle plz";

    core::StringRange<char> range(nullptr, nullptr);
    core::StringRange<char> range1(text, text + core::strUtil::strlen(text));
    core::StringRange<char> range2(text, text + core::strUtil::strlen(text) / 2);
    core::StringRange<char> range3(text, core::strUtil::strlen(text) - 1);

    EXPECT_TRUE((text + 10) == range1.find("pickle"));
    EXPECT_TRUE(nullptr == range2.find("plz"));
    EXPECT_TRUE(nullptr == range3.find("plz"));
    EXPECT_TRUE((text + 17) == range3.find("pl"));
}

TEST(StringRange, FindW)
{
    wchar_t text[21] = L"tickle my pickle plz";

    core::StringRange<wchar_t> range(nullptr, nullptr);
    core::StringRange<wchar_t> range1(text, text + core::strUtil::strlen(text));
    core::StringRange<wchar_t> range2(text, text + core::strUtil::strlen(text) / 2);
    core::StringRange<wchar_t> range3(text, core::strUtil::strlen(text) - 1);

    EXPECT_TRUE((text + 10) == range1.find(L"pickle"));
    EXPECT_TRUE(nullptr == range2.find(L"plz"));
    EXPECT_TRUE(nullptr == range3.find(L"plz"));
    EXPECT_TRUE((text + 17) == range3.find(L"pl"));
}

TEST(StringRange, FindWhiteSpace)
{
    char text[21] = "tickle my pickle plz";

    core::StringRange<char> range(nullptr, nullptr);
    core::StringRange<char> range1(text, text + core::strUtil::strlen(text));
    core::StringRange<char> range2(text, text + core::strUtil::strlen(text) / 2);
    core::StringRange<char> range3(text, core::strUtil::strlen(text) - 1);

    EXPECT_TRUE((text + 6) == range1.findWhitespace());
    EXPECT_TRUE((text + 6) == range2.findWhitespace());
    EXPECT_TRUE((text + 6) == range3.findWhitespace());
    EXPECT_TRUE((text + 6) == range3.findWhitespace());
}

TEST(StringRange, FindWhiteSpaceW)
{
    wchar_t text[21] = L"tickle my pickle plz";

    core::StringRange<wchar_t> range(nullptr, nullptr);
    core::StringRange<wchar_t> range1(text, text + core::strUtil::strlen(text));
    core::StringRange<wchar_t> range2(text, text + core::strUtil::strlen(text) / 2);
    core::StringRange<wchar_t> range3(text, core::strUtil::strlen(text) - 1);

    EXPECT_TRUE((text + 6) == range1.findWhitespace());
    EXPECT_TRUE((text + 6) == range2.findWhitespace());
    EXPECT_TRUE((text + 6) == range3.findWhitespace());
    EXPECT_TRUE((text + 6) == range3.findWhitespace());
}

TEST(StringRange, FindNonWhitespace)
{
    char text[22] = " tickle my pickle plz";

    core::StringRange<char> range1(text, text + core::strUtil::strlen(text));
    core::StringRange<char> range2(text + 7, text + core::strUtil::strlen(text) / 2);
    core::StringRange<char> range3(text + 10, core::strUtil::strlen(text) - 1);

    EXPECT_TRUE((text + 1) == range1.findNonWhitespace());
    EXPECT_TRUE((text + 8) == range2.findNonWhitespace());
    EXPECT_TRUE((text + 11) == range3.findNonWhitespace());
}

TEST(StringRange, FindNonWhitespaceW)
{
    wchar_t text[22] = L" tickle my pickle plz";

    core::StringRange<wchar_t> range1(text, text + core::strUtil::strlen(text));
    core::StringRange<wchar_t> range2(text + 7, text + core::strUtil::strlen(text) / 2);
    core::StringRange<wchar_t> range3(text + 10, core::strUtil::strlen(text) - 1);

    EXPECT_TRUE((text + 1) == range1.findNonWhitespace());
    EXPECT_TRUE((text + 8) == range2.findNonWhitespace());
    EXPECT_TRUE((text + 11) == range3.findNonWhitespace());
}

TEST(StringRange, ArrayOperator)
{
    char text[22] = " tickle my pickle plz";

    core::StringRange<char> range1(text, text + core::strUtil::strlen(text));

    EXPECT_EQ(' ', range1[0]);
    EXPECT_EQ('t', range1[1]);
    EXPECT_EQ('z', range1[20]);

    core::debugging::EnableBreakpoints(false);
    g_AssetChecker.ExpectAssertion(true);

    // out of range.
    char val = range1[21];

    EXPECT_TRUE(g_AssetChecker.HadCorrectAssertions());

    g_AssetChecker.ExpectAssertion(false);
    core::debugging::EnableBreakpoints(true);
}

TEST(StringRange, ArrayOperatorW)
{
    wchar_t text[22] = L" tickle my pickle plz";

    core::StringRange<wchar_t> range1(text, text + core::strUtil::strlen(text));

    EXPECT_EQ(L' ', range1[0]);
    EXPECT_EQ(L't', range1[1]);
    EXPECT_EQ(L'z', range1[20]);

    core::debugging::EnableBreakpoints(false);
    g_AssetChecker.ExpectAssertion(true);

    // out of range.
    wchar_t val = range1[21];

    EXPECT_TRUE(g_AssetChecker.HadCorrectAssertions());

    g_AssetChecker.ExpectAssertion(false);
    core::debugging::EnableBreakpoints(true);
}
