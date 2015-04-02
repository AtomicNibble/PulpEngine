#include "stdafx.h"

#include "gtest/gtest.h"

#include <String\GrowingStringTable.h>

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

}


TEST(GrowingStringTable, Add)
{
	GrowingStringTable<128, 16, 8, uint16_t> Table(g_arena);

	size_t i;
	uint16_t ids[10];
	const char* pPointers[10];

	for (i = 0; i < 10; i++)
	{
		ids[i] = Table.addString(pStrings[i]);

		bool validId = (ids[i] != GrowingStringTable<128, 16, 8, uint16_t>::InvalidId);

		ASSERT_TRUE(validId);
	}

	ASSERT_EQ(Table.numStrings(), 10);

	for (i = 0; i < 10; i++)
	{
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

	for (i = 0; i < 20; i++)
	{
		uint8_t id = Table.addString(pStrings[i % 10]);

		bool validId = (id != TableType::InvalidId);

		if(!validId) {
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


