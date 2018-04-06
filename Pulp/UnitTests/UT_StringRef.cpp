#include "stdafx.h"

#include <String\StrRef.h>

X_USING_NAMESPACE;

using namespace core;

typedef ::testing::Types<char> MyTypes;
TYPED_TEST_CASE(StrRef, MyTypes);

template<typename T>
class StrRef : public ::testing::Test
{
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
    StrRefT fbeginend(begin, end);

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
    typedef StringRef<TypeParam> StrRefT;

    StrRefT str;
    StrRefT str1("camel");
    StrRefT str2("potato");
    StrRefT str3("cat");

    str.append("goat");
    EXPECT_STREQ("goat", str);

    str.append("man", 2);
    EXPECT_STREQ("goatma", str);

    str.append(str1, 1, 3);
    EXPECT_STREQ("goatmaame", str);

    str.append(str2);
    EXPECT_STREQ("goatmaamepotato", str);

    str.append(6, '@');
    EXPECT_STREQ("goatmaamepotato@@@@@@", str);

    str.append(str3.begin(), str3.begin()); // should add nothing.
    EXPECT_STREQ("goatmaamepotato@@@@@@", str);

    str.append(str3.begin(), str3.end()); // actualy append it now.
    EXPECT_STREQ("goatmaamepotato@@@@@@cat", str);
}

TYPED_TEST(StrRef, Assign)
{
    typedef StringRef<TypeParam> StrRefT;

    StrRefT str;
    StrRefT str1("camel");
    StrRefT str2("potato");
    StrRefT str3("cat");

    str.assign("goat");
    EXPECT_STREQ("goat", str);

    str.assign("man", 2);
    EXPECT_STREQ("ma", str);

    str.assign(str1, 1, 3);
    EXPECT_STREQ("ame", str);

    str.assign(str2);
    EXPECT_STREQ("potato", str);

    str.assign(6, '@');
    EXPECT_STREQ("@@@@@@", str);

    str.assign(str3.begin(), str3.begin()); // should add nothing.
    EXPECT_STREQ("", str);

    str.assign(str3.begin(), str3.end()); // actualy append it now.
    EXPECT_STREQ("cat", str);
}

TYPED_TEST(StrRef, Replace)
{
    typedef StringRef<TypeParam> StrRefT;

    StrRefT str("camel likes to ride his bike");

    str.replace('i', 'g');
    EXPECT_STREQ("camel lgkes to rgde hgs bgke", str);

    str.replace("camel", "goat");
    EXPECT_STREQ("goat lgkes to rgde hgs bgke", str);

    str.replace(6, 4, "ikes");
    EXPECT_STREQ("goat likes to rgde hgs bgke", str);

    str.replace(23, 4, "camelllls", 5);
    EXPECT_STREQ("goat likes to rgde hgs camel", str);

    str.replace(0, 4, 10, '#');
    EXPECT_STREQ("########## likes to rgde hgs camel", str);

    // provide a length longer than new string
    str.replace(20, 4, "camelllls", 50);
    EXPECT_STREQ("########## likes to camelllls", str);
}

TYPED_TEST(StrRef, Insert)
{
    typedef StringRef<TypeParam> StrRefT;

    StrRefT str("came to his shp");

    str.insert(4, 'l');
    EXPECT_STREQ("camel to his shp", str);

    str.insert(15, 2, 'e');
    EXPECT_STREQ("camel to his sheep", str);

    str.insert(9, "ride ");
    EXPECT_STREQ("camel to ride his sheep", str);

    str.insert(6, "likes cakes", 6);
    EXPECT_STREQ("camel likes to ride his sheep", str);
}

TYPED_TEST(StrRef, Erase)
{
    typedef StringRef<TypeParam> StrRefT;

    StrRefT str("camel likes to ride his sheep");

    str.erase(0, 6);
    EXPECT_STREQ("likes to ride his sheep", str);

    str.erase(17);
    EXPECT_STREQ("likes to ride his", str);
}

TYPED_TEST(StrRef, Compare)
{
    typedef StringRef<TypeParam> StrRefT;

    StrRefT str("goat");
    StrRefT str1("goat");
    StrRefT str2("goot");

    EXPECT_TRUE(str.compare(str1));
    EXPECT_TRUE(str.compare("goat"));

    EXPECT_FALSE(str.compare(str2));
    EXPECT_FALSE(str.compare("goot"));
}

TYPED_TEST(StrRef, CompareInt)
{
    typedef StringRef<TypeParam> StrRefT;

    StrRefT str("goat");
    StrRefT str1("goat");
    StrRefT str2("goot");
    StrRefT str3("goaa");

    EXPECT_EQ(0, str.compareInt(str1));
    EXPECT_EQ(0, str.compareInt("goat"));

    // it's 0 vs a so negative
    EXPECT_EQ(-1, str.compareInt(str2));
    EXPECT_EQ(-1, str.compareInt("goot"));

    // do some positive ones.
    EXPECT_EQ(1, str.compareInt(str3));
    EXPECT_EQ(1, str.compareInt("goaa"));
}

TYPED_TEST(StrRef, CompareNoCase)
{
    typedef StringRef<TypeParam> StrRefT;

    StrRefT str("goat");
    StrRefT str1("GOaT");
    StrRefT str2("GOot");

    EXPECT_TRUE(str.compareCaseInsen(str1));
    EXPECT_TRUE(str.compareCaseInsen("GOaT"));

    EXPECT_FALSE(str.compareCaseInsen(str2));
    EXPECT_FALSE(str.compareCaseInsen("GOot"));
}

TYPED_TEST(StrRef, Find)
{
    typedef StringRef<TypeParam> StrRefT;

    StrRefT str("goat likes to float in a moat");

    EXPECT_EQ(str.begin() + 5, str.find("likes"));
    EXPECT_EQ(str.begin() + 6, str.find('i'));

    EXPECT_EQ(nullptr, str.find("ina"));
    EXPECT_EQ(nullptr, str.find('x'));
}

TYPED_TEST(StrRef, Swap)
{
    typedef StringRef<TypeParam> StrRefT;

    StrRefT str("goat");
    StrRefT str1("boat");

    str.swap(str1);

    EXPECT_STREQ("goat", str1);
    EXPECT_STREQ("boat", str);
}

TYPED_TEST(StrRef, SubStr)
{
    typedef StringRef<TypeParam> StrRefT;

    StrRefT str("goat man"), str1, str2;
    StrRefT::const_str pos = str.find("man");

    str1 = str.substr(nullptr, pos);

    EXPECT_STREQ("goat man", str);
    EXPECT_STREQ("goat ", str1);

    str2 = str.substr(pos);

    EXPECT_STREQ("goat man", str);
    EXPECT_STREQ("man", str2);
}

TYPED_TEST(StrRef, Left)
{
    typedef StringRef<TypeParam> StrRefT;

    StrRefT str("goat man"), str1, str2;

    str1 = str.left(2);
    str2 = str.left(200); // give all

    EXPECT_STREQ("goat man", str);
    EXPECT_STREQ("goat man", str2);
    EXPECT_STREQ("go", str1);
}

TYPED_TEST(StrRef, Right)
{
    typedef StringRef<TypeParam> StrRefT;

    StrRefT str("goat man"), str1, str2;

    str1 = str.right(2);
    str2 = str.right(200); // give all

    EXPECT_STREQ("goat man", str);
    EXPECT_STREQ("goat man", str2);
    EXPECT_STREQ("an", str1);
}

TYPED_TEST(StrRef, OPAssign)
{
    typedef StringRef<TypeParam> StrRefT;

    StrRefT str("goat man");
    StrRefT str1("candy in your nipple");

    str = str1;

    EXPECT_STREQ("candy in your nipple", str.c_str());

    str = 'g';

    EXPECT_STREQ("g", str.c_str());

    str = "lemon wall";

    EXPECT_STREQ("lemon wall", str.c_str());
}

TYPED_TEST(StrRef, OPConcat)
{
    typedef StringRef<TypeParam> StrRefT;

    StrRefT str("goat man");
    StrRefT str1(" likes t");

    str += str1;

    EXPECT_STREQ("goat man likes t", str.c_str());

    str += 'o';

    EXPECT_STREQ("goat man likes to", str.c_str());

    str += " play with your nipples";

    EXPECT_STREQ("goat man likes to play with your nipples", str.c_str());
}
