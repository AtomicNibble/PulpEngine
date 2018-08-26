#include "stdafx.h"

X_USING_NAMESPACE;

TEST(StackString, stripColorCodes)
{
    core::StackString<64> str_1("hel^1lo ^8bob");
    core::StackString<64> str_2("hello bob^1^what^9 is your ^1name");
    core::StackString<64> str_3("hello  2goat cow boat bob^");
    core::StackString<64> str_4("hello^4 sfs sebob");
    core::StackString<64> str_5("^1hello adad a abob^2");
    core::StackString<64> str_6("hellof^4sf se bob");
    core::StackString<64> str_7("helf fsel^8o bob");
    core::StackString<64> str_8("hellfsf ^7^&^&^9fso bob");
    core::StackString<64> str_9("hellofse bob^^^^^");

    str_1.stripColorCodes();
    str_2.stripColorCodes();
    str_3.stripColorCodes();
    str_4.stripColorCodes();
    str_5.stripColorCodes();
    str_6.stripColorCodes();
    str_7.stripColorCodes();
    str_8.stripColorCodes();
    str_9.stripColorCodes();

    EXPECT_STREQ("hello bob", str_1.c_str());
    EXPECT_STREQ("hello bob^what is your name", str_2.c_str());
    EXPECT_STREQ("hello  2goat cow boat bob^", str_3.c_str());
    EXPECT_STREQ("hello sfs sebob", str_4.c_str());
    EXPECT_STREQ("hello adad a abob", str_5.c_str());
    EXPECT_STREQ("hellofsf se bob", str_6.c_str());
    EXPECT_STREQ("helf fselo bob", str_7.c_str());
    EXPECT_STREQ("hellfsf ^&^&fso bob", str_8.c_str());
    EXPECT_STREQ("hellofse bob^^^^^", str_9.c_str());
}

TEST(StackString, Construct)
{
    auto wideStr = L"cow goes meow";

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
    core::StackString<32> str32_wide(L"meow meow");
    core::StackString<32> str32_wide_len(wideStr, wideStr + core::strUtil::strlen(wideStr));

    core::StringRange<char> range(str32_str.begin(), str32_str.end());

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
    EXPECT_STREQ("meow meow", str32_wide.c_str());
    EXPECT_STREQ("cow goes meow", str32_wide_len.c_str());
}

TEST(StackString, Set)
{
    core::StackString<1024> str("goat man");

    str.set("Lorem Ipsum is simply dummy text of the printing and"
            " typesetting industry.Lorem Ipsum has been the industry's"
            " standard dummy text ever since the 1500");

    EXPECT_STREQ("Lorem Ipsum is simply dummy text of the printing and"
                 " typesetting industry.Lorem Ipsum has been the industry's"
                 " standard dummy text ever since the 1500",
        str.c_str());

    const char testTxt[38] = {"slap a goat in a moat while in a boat"};

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

    const char testTxt[10] = {"hookersss"};
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

TEST(StackString, TrimWhiteSpaceLeft)
{
    core::StackString<1024> str("    show me your whitespace    ");

    core::StackString<1024> str1 = str.trimLeft();
    EXPECT_STREQ("show me your whitespace    ", str.c_str());
    EXPECT_STREQ("show me your whitespace    ", str1.c_str());
}

TEST(StackString, TrimWhiteSpaceRight)
{
    core::StackString<1024> str("    show me your whitespace    ");

    core::StackString<1024> str1 = str.trimRight();
    EXPECT_STREQ("    show me your whitespace", str.c_str());
    EXPECT_STREQ("    show me your whitespace", str1.c_str());
}

TEST(StackString, TrimWhiteSpaceLeftRight)
{
    core::StackString<1024> str("    show me your whitespace    ");

    core::StackString<1024> str1 = str.trim();
    EXPECT_STREQ("show me your whitespace", str.c_str());
    EXPECT_STREQ("show me your whitespace", str1.c_str());
}

TEST(StackString, trimCharacter)
{
    core::StackString<1024> str("--- -----show me your whitespace------");

    str.trim(' ');
    EXPECT_STREQ("--- -----show me your whitespace------", str.c_str());

    str.trim('-');
    EXPECT_STREQ(" -----show me your whitespace", str.c_str());
    str.trim(' ');
    EXPECT_STREQ("-----show me your whitespace", str.c_str());

    str.trim('-');
    EXPECT_STREQ("show me your whitespace", str.c_str());
}

TEST(StackString, trimLeft)
{
    core::StackString<1024> str("--- -----show me your whitespace------");

    str.trimLeft('w');
    EXPECT_STREQ("--- -----show me your whitespace------", str.c_str());
    str.trimLeft('-');

    EXPECT_STREQ(" -----show me your whitespace------", str.c_str());

    str.trimLeft(str.c_str() + 10);
    EXPECT_STREQ(" me your whitespace------", str.c_str());
}

TEST(StackString, trimRight)
{
    core::StackString<1024> str("--- -----show me your whitespace------");

    str.trimRight('w');
    EXPECT_STREQ("--- -----show me your whitespace------", str.c_str());
    str.trimRight('-');

    EXPECT_STREQ("--- -----show me your whitespace", str.c_str());

    str.trimRight(str.c_str() + 10);
    EXPECT_STREQ("--- -----s", str.c_str());
}

TEST(StackString, stripTrailing)
{
    core::StackString<1024> str("--- -----show me your whitespace------");

    str.stripTrailing('e');
    EXPECT_STREQ("--- -----show me your whitespace------", str.c_str());

    str.stripTrailing('-');
    EXPECT_STREQ("--- -----show me your whitespace", str.c_str());
}

TEST(StackString, Clear)
{
    core::StackString<1024> str("When the goat is in the moat it's blocked");

    str.clear();

    EXPECT_EQ(0, str.length());
    EXPECT_EQ(1024, str.capacity());
}

TEST(StackString, isEqual)
{
    core::StackString<64> str("goat");
    core::StackString<64> str1("goAt");
    core::StackString<64> str2("goat");

    EXPECT_FALSE(str.isEqual(str1.c_str()));
    EXPECT_FALSE(str.isEqual("gOat"));

    EXPECT_TRUE(str.isEqual(str2.c_str()));
    EXPECT_TRUE(str.isEqual("goat"));
}

TEST(StackString, findLast)
{
    core::StackString<128> str("ooogoogodoghdooogdgoooat");

    const char* pFind = str.findLast('o');
    EXPECT_TRUE(pFind != nullptr);
    EXPECT_TRUE(pFind == (str.end() - 3));

    EXPECT_TRUE(str.findLast('x') == nullptr);
}

TEST(StackString, find)
{
    core::StackString<128> str("ooogoogodoghdooogdgoooat");

    const char* pFind = str.find('g');
    EXPECT_TRUE(pFind != nullptr);
    EXPECT_TRUE(pFind == (str.begin() + 3));

    EXPECT_TRUE(str.find('x') == nullptr);

    const char* pFind2 = str.find("gooo");
    EXPECT_TRUE(pFind2 != nullptr);
    EXPECT_TRUE(pFind2 == (str.begin() + 18));
}

TEST(StackString, findCaseInsen)
{
    core::StackString<128> str("ooogoogodoghdXooogdGoooat");

    const char* pFind = str.findCaseInsen('g');
    EXPECT_TRUE(pFind != nullptr);
    EXPECT_TRUE(pFind == (str.begin() + 3));

    EXPECT_TRUE(str.findCaseInsen('x') != nullptr);
    EXPECT_TRUE(str.findCaseInsen('X') != nullptr);
    EXPECT_TRUE(str.findCaseInsen('y') == nullptr);
    EXPECT_TRUE(str.findCaseInsen('Y') == nullptr);

    const char* pFind2 = str.findCaseInsen("Gooo");
    EXPECT_TRUE(pFind2 != nullptr);
    EXPECT_TRUE(pFind2 == (str.begin() + 19));
}

TEST(StackString, operatorEqual)
{
    core::StackString<64> str("goat");
    core::StackString<64> str1("goat");
    core::StackString<64> str2("goaT");

    EXPECT_TRUE(str == str1);
    EXPECT_FALSE(str == str2);
}

TEST(StackString, operatorNotEqual)
{
    core::StackString<64> str("goat");
    core::StackString<64> str1("goat");
    core::StackString<64> str2("goaT");

    EXPECT_FALSE(str != str1);
    EXPECT_TRUE(str != str2);
}

TEST(StackString, operatorAssign)
{
    core::StackString<64> str("goat");
    core::StackString<64> str2("goaT");

    str = str2;

    EXPECT_STREQ("goaT", str.c_str());
}

TEST(StackString, operatorArray)
{
    core::StackString<64> str("goat");

    EXPECT_EQ('g', str[0]);
    EXPECT_EQ('o', str[1]);
    EXPECT_EQ('a', str[2]);
    EXPECT_EQ('t', str[3]);

    // i allow access to null term.
    EXPECT_EQ('\0', str[4]);

    core::debugging::EnableBreakpoints(false);
    g_AssetChecker.ExpectAssertion(true);

    // out of range.
    char val = str[5];

    EXPECT_TRUE(g_AssetChecker.HadCorrectAssertions());

    g_AssetChecker.ExpectAssertion(false);
    core::debugging::EnableBreakpoints(true);
}

TEST(StackString, c_str)
{
    core::StackString<64> str("goat");
    core::StackString<64> str1("");
    core::StackString<32> str2("thistring is too long for me ee");

    EXPECT_STREQ("goat", str.c_str());
    EXPECT_STREQ("", str1.c_str());
    EXPECT_STREQ("thistring is too long for me ee", str2.c_str());
}

TEST(StackString, length)
{
    core::StackString<64> str("goat");
    core::StackString<64> str1("");
    core::StackString<32> str2("thistring is too long for me ee");

    EXPECT_EQ(4, str.length());
    EXPECT_EQ(0, str1.length());
    EXPECT_EQ(31, str2.length());
}

TEST(StackString, capacity)
{
    core::StackString<64> str("goat");
    core::StackString<64> str1("");
    core::StackString<32> str2("thistring is too long for me ee");

    EXPECT_EQ(64, str.capacity());
    EXPECT_EQ(64, str1.capacity());
    EXPECT_EQ(32, str2.capacity());
}

TEST(StackString, isEmpty)
{
    core::StackString<64> str("goat");
    core::StackString<64> str1("");

    EXPECT_FALSE(str.isEmpty());
    EXPECT_TRUE(str1.isEmpty());

    str.clear();

    EXPECT_TRUE(str.isEmpty());
}

TEST(StackString, isNotEmpty)
{
    core::StackString<64> str("goat");
    core::StackString<64> str1("");

    EXPECT_TRUE(str.isNotEmpty());
    EXPECT_FALSE(str1.isNotEmpty());

    str.clear();

    EXPECT_FALSE(str.isNotEmpty());
}

TEST(StackString, toLower)
{
    core::StackString<64> str("goat");
    core::StackString<64> str1("GOATMAN");
    core::StackString<64> str2("GoaTMaN");

    EXPECT_STREQ("GoaTMaN", str2.c_str());

    str.toLower();
    str1.toLower();
    str2.toLower();

    EXPECT_STREQ("goat", str.c_str());
    EXPECT_STREQ("goatman", str1.c_str());
    EXPECT_STREQ("goatman", str2.c_str());
}

TEST(StackString, toUpper)
{
    core::StackString<64> str("goat");
    core::StackString<64> str1("goatMan");
    core::StackString<64> str2("GoaTMaN");

    EXPECT_STREQ("GoaTMaN", str2.c_str());

    str.toUpper();
    str1.toUpper();
    str2.toUpper();

    EXPECT_STREQ("GOAT", str.c_str());
    EXPECT_STREQ("GOATMAN", str1.c_str());
    EXPECT_STREQ("GOATMAN", str2.c_str());
}

TEST(StackString, begin)
{
    core::StackString<64> str("goat");
    core::StackString<64> str1("goatMan");
    core::StackString<64> str2("GoaTMaN");

    EXPECT_STREQ("goat", str.begin());
    EXPECT_STREQ("goatMan", str1.begin());
    EXPECT_STREQ("GoaTMaN", str2.begin());
}

TEST(StackString, end)
{
    core::StackString<64> str("goat");
    core::StackString<64> str1("goatMan");
    core::StackString<64> str2("GoaTMaN");

    EXPECT_EQ(str.begin() + 4, str.end());
    EXPECT_EQ(str1.begin() + 7, str1.end());
    EXPECT_EQ(str2.begin() + 7, str2.end());
}