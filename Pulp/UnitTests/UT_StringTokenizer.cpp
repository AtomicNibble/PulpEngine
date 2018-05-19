#include "stdafx.h"

#include <String\StringRange.h>
#include <String\StringTokenizer.h>

X_USING_NAMESPACE;

// very basic test. needs expanding.

TEST(StringTokenizer, Narrow)
{
    const char text[] = "variableName = 10";

    core::StringTokenizer<char> tokenizer(text, text + sizeof(text), L'=');
    core::StringRange<char> valueName(nullptr, nullptr);
    const bool foundValueName = tokenizer.extractToken(valueName);
    core::StringRange<char> value(nullptr, nullptr);
    const bool foundValue = tokenizer.extractToken(value);

    ASSERT_TRUE(foundValueName);
    ASSERT_TRUE(foundValue);

    core::StackString<64, char> valueNameStr(valueName);
    EXPECT_STREQ("variableName", valueNameStr.c_str());
    core::StackString<64, char> valueStr(value);
    EXPECT_STREQ("10", valueStr.c_str());
}

TEST(StringTokenizer, Wide)
{
    const wchar_t text[] = L"variableName = 10";

    core::StringTokenizer<wchar_t> tokenizer(text, text + sizeof(text), L'=');
    core::StringRange<wchar_t> valueName(nullptr, nullptr);
    const bool foundValueName = tokenizer.extractToken(valueName);
    core::StringRange<wchar_t> value(nullptr, nullptr);
    const bool foundValue = tokenizer.extractToken(value);

    ASSERT_TRUE(foundValueName);
    ASSERT_TRUE(foundValue);

    core::StackString<64, wchar_t> valueNameStr(valueName);
    EXPECT_STREQ(L"variableName", valueNameStr.c_str());
    core::StackString<64, wchar_t> valueStr(value);
    EXPECT_STREQ(L"10", valueStr.c_str());
}