#include "stdafx.h"

#include <IFileSys.h>

#include <String\GrowingStringTable.h>
#include <Memory/AllocationPolicies/StackAllocator.h>

X_USING_NAMESPACE;

using namespace core;

namespace
{
    const char* pStrings[10] = {
        "j_gun",
        "tag_flash",
        "this string is quite long baby, show me your blocks.",
        "how many nippels can you see",
        "i've fallen over. help",
        "Click to start the rape",
        "6p",
        "123456789AB",
        "Meow",
        "Goat man on the loose!",
    };

    const char* pStringsDuplicates[20] = {
        "j_gun",
        "tag_flash",
        "123456789AB",
        "j_gun",
        "this string is quite long baby, show me your blocks.",
        "how many nippels can you see",
        "how many nippels can you see",
        "how many nippels can you see",
        "how many nippels can you see",
        "i've fallen over. help",
        "Click to start the rape",
        "6p",
        "123456789AB",
        "j_gun",
        "j_gun",
        "j_gun",
        "j_gun",
        "Meow",
        "Goat man on the loose!",
        "j_gun",
    };
} // namespace

TEST(GrowingStringTable, Add)
{
    GrowingStringTable<128, 16, 8, uint16_t> Table(g_arena);

    size_t i;
    uint16_t ids[10];
    const char* pPointers[10];

    for (i = 0; i < 10; i++) {
        ids[i] = Table.addString(pStrings[i]);

        bool validId = (ids[i] != GrowingStringTable<128, 16, 8, uint16_t>::InvalidId);

        ASSERT_TRUE(validId);
    }

    ASSERT_EQ(Table.numStrings(), 10);

    for (i = 0; i < 10; i++) {
        pPointers[i] = Table.getString(ids[i]);
        // check it's aligned
        EXPECT_TRUE(core::pointerUtil::IsAligned(pPointers[i], 8, 0));
        // check it's the correct string.
        EXPECT_STREQ(pStrings[i], pPointers[i]);
    }
}

TEST(GrowingStringTable, AddOverFlow)
{
    // max out at 256 blocks.
    // make the blocks small so we max out quicker.
    typedef GrowingStringTable<128, 4, 4, uint8_t> TableType;
    TableType Table(g_arena);
    bool failed = false;
    size_t i;

    for (i = 0; i < 20; i++) {
        uint8_t id = Table.addString(pStrings[i % 10]);

        bool validId = (id != TableType::InvalidId);

        if (!validId) {
            EXPECT_EQ(10, i);
            EXPECT_EQ(10, Table.numStrings());
            failed = true;
            break;
        }
    }

    EXPECT_TRUE(failed);
}

TEST(GrowingStringTable, InvalidBlockSize)
{
    core::debugging::EnableBreakpoints(false);
    g_AssetChecker.ExpectAssertion(true);

    typedef GrowingStringTable<128, 0, 4, uint8_t> TableType;
    TableType Table(g_arena);

    EXPECT_TRUE(g_AssetChecker.HadCorrectAssertions());

    g_AssetChecker.ExpectAssertion(false);
    core::debugging::EnableBreakpoints(true);
}

TEST(GrowingStringTable, InvalidBlockSize2)
{
    core::debugging::EnableBreakpoints(false);
    g_AssetChecker.ExpectAssertion(true);

    typedef GrowingStringTable<128, 3, 4, uint8_t> TableType;
    TableType Table(g_arena);

    EXPECT_TRUE(g_AssetChecker.HadCorrectAssertions());

    g_AssetChecker.ExpectAssertion(false);
    core::debugging::EnableBreakpoints(true);
}

TEST(GrowingStringTable, InvalidAlignment)

{
    core::debugging::EnableBreakpoints(false);
    g_AssetChecker.ExpectAssertion(true);

    typedef GrowingStringTable<128, 70, 70, uint8_t> TableType;
    TableType Table(g_arena);

    EXPECT_TRUE(g_AssetChecker.HadCorrectAssertions());

    g_AssetChecker.ExpectAssertion(false);
    core::debugging::EnableBreakpoints(true);
}

TEST(GrowingStringTable, InvalidGran)
{
    core::debugging::EnableBreakpoints(false);
    g_AssetChecker.ExpectAssertion(true);

    typedef GrowingStringTable<0, 4, 4, uint8_t> TableType;
    TableType Table(g_arena);

    EXPECT_TRUE(g_AssetChecker.HadCorrectAssertions());

    g_AssetChecker.ExpectAssertion(false);
    core::debugging::EnableBreakpoints(true);
}

TEST(GrowingStringTable, Serialize)
{
    GrowingStringTable<128, 16, 8, uint16_t> Table(g_arena);

    size_t i;
    uint16_t ids[10];
    const char* pPointers[10];

    for (i = 0; i < 10; i++) {
        ids[i] = Table.addString(pStrings[i]);
        bool validId = (ids[i] != GrowingStringTable<128, 16, 8, uint16_t>::InvalidId);
        ASSERT_TRUE(validId);
    }

    ASSERT_EQ(Table.numStrings(), 10);

    for (i = 0; i < 10; i++) {
        pPointers[i] = Table.getString(ids[i]);
        // check it's aligned
        EXPECT_TRUE(core::pointerUtil::IsAligned(pPointers[i], 8, 0));
        // check it's the correct string.
        EXPECT_STREQ(pStrings[i], pPointers[i]);
    }

    // save to file then read back.
    ASSERT_TRUE(NULL != gEnv->pFileSys);
    IFileSys* pFileSys = gEnv->pFileSys;

    core::Path<char> fileName;
    fileName += X_ENGINE_NAME;
    fileName += "_ut_GrowingStringTable_serialize_Type(";
    fileName += "uint16_t";
    fileName += ").ut_dat";

    XFile* file = pFileSys->openFile(fileName,
        FileFlag::WRITE | FileFlag::READ | FileFlag::RECREATE | FileFlag::RANDOM_ACCESS);
    ASSERT_TRUE(NULL != file);
    if (file) {
        ASSERT_TRUE(Table.SSave(file));

        Table.free();
        Table.addString("tickle my pickle one more time!.");

        file->seek(0, SeekMode::SET);

        ASSERT_TRUE(Table.SLoad(file));

        // check is valid after the load.
        ASSERT_EQ(Table.numStrings(), 10);

        for (i = 0; i < 10; i++) {
            pPointers[i] = Table.getString(ids[i]);
            // check it's aligned
            EXPECT_TRUE(core::pointerUtil::IsAligned(pPointers[i], 8, 0));
            // check it's the correct string.
            EXPECT_STREQ(pStrings[i], pPointers[i]);
        }

        ASSERT_EQ(0, file->remainingBytes());
        pFileSys->closeFile(file);
    }
}

// ==============================================

TEST(GrowingStringTableUnique, Add)
{
    MallocFreeAllocator allocator;

    typedef core::MemoryArena<
        core::MallocFreeAllocator,
        core::SingleThreadPolicy,
#if X_ENABLE_MEMORY_DEBUG_POLICIES
        core::SimpleBoundsChecking,
        core::SimpleMemoryTracking,
        core::SimpleMemoryTagging
#else
        core::NoBoundsChecking,
        core::NoMemoryTracking,
        core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
        >
        StackArena;

    StackArena arena(&allocator, "StingTableArena");

    typedef GrowingStringTableUnique<128, 32, 32, uint16_t> TableType;
    TableType Table(&arena);

    size_t i;
    uint16_t ids[20];
    const char* pPointers[20];

    for (i = 0; i < 20; i++) {
        ids[i] = Table.addStringUnqiue(pStringsDuplicates[i]);

        bool validId = (ids[i] != TableType::InvalidId);

        ASSERT_TRUE(validId);
    }

    ASSERT_EQ(Table.numStrings(), 10);

    for (i = 0; i < 20; i++) {
        pPointers[i] = Table.getString(ids[i]);
        // check it's aligned
        EXPECT_TRUE(core::pointerUtil::IsAligned(pPointers[i], 32, 0));
        // check it's the correct string.
        EXPECT_STREQ(pStringsDuplicates[i], pPointers[i]);
    }

    Table.free();

    // check no leaks
    EXPECT_EQ(0, allocator.getStatistics().allocationCount_);
}