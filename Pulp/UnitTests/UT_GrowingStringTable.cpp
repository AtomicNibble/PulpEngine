#include "stdafx.h"

#include "gtest/gtest.h"

#include <String\GrowingStringTable.h>

X_USING_NAMESPACE;

using namespace core;



TEST(GrowingStringTable, Add)
{
	GrowingStringTable<128, 16, 8, uint16_t> Table(g_arena);

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
	
	int pad = 0;
}