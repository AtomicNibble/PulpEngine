#include "stdafx.h"

#include <String\StringTable.h>

X_USING_NAMESPACE;

using namespace core;

TEST(StringTable, Add)
{
    StringTable<128, 16, 8, uint16_t> Table;

    uint16_t id1 = Table.addString("j_gun");
    uint16_t id2 = Table.addString("tag_flash");
    uint16_t id3 = Table.addString("this string is quite long baby, show me your blocks.");
    uint16_t id4 = Table.addString("how many nippels can you see");
    uint16_t id5 = Table.addString("i've fallen over. help");
    uint16_t id6 = Table.addString("Click to start the rape");
    uint16_t id7 = Table.addString("6p");
    uint16_t id8 = Table.addString("123456789AB");
    uint16_t id9 = Table.addString("Meow");

    EXPECT_EQ(0, id1);
    EXPECT_EQ(1, id2);
    EXPECT_EQ(2, id3);
    EXPECT_EQ(6, id4);
    EXPECT_EQ(9, id5);
    EXPECT_EQ(11, id6);
    EXPECT_EQ(13, id7);
    EXPECT_EQ(14, id8);
    EXPECT_EQ(9, Table.numStrings());
}

TEST(StringTable, AddUnique)
{
    StringTableUnique<128, 16, 8, uint16_t> Table(g_arena);

    uint16_t id1 = Table.addStringUnqiue("i've fallen over. help");
    uint16_t id2 = Table.addStringUnqiue("Click to start the rape");
    uint16_t id3 = Table.addStringUnqiue("6p");
    uint16_t id4 = Table.addStringUnqiue("123456789AB");
    uint16_t id5 = Table.addStringUnqiue("Meow");
    uint16_t id6 = Table.addStringUnqiue("tag_flash");
    uint16_t id7 = Table.addStringUnqiue("j_gun");
    uint16_t id8 = Table.addStringUnqiue("j_gun");
    uint16_t id9 = Table.addStringUnqiue("j_gun1");

    const char* str = Table.getString(id7);

    size_t waste = Table.wastedBytes();

    EXPECT_EQ(0, id1);
    EXPECT_EQ(2, id2);
    EXPECT_EQ(4, id3);
    EXPECT_EQ(5, id4);
    EXPECT_EQ(6, id5);
    EXPECT_EQ(7, id6);
    EXPECT_EQ(8, id7);
    EXPECT_EQ(8, id8);
    EXPECT_EQ(9, id9);

    EXPECT_EQ(id7, id8);
    EXPECT_NE(id7, id9);
    EXPECT_STREQ("j_gun", str);
    EXPECT_EQ(8, Table.numStrings());
}