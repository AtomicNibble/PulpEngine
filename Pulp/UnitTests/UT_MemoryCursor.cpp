#include "stdafx.h"

#include "gtest/gtest.h"

#include <Memory\MemCursor.h>

X_USING_NAMESPACE;

using namespace core;


TEST(MemCusor, Seek) {

	int pData[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

	core::MemCursor<int> cursor(pData, 10);

	cursor.Seek<int>(5);

	EXPECT_EQ(cursor.get<int>(), 5);

	cursor.Seek<int>(5);

	EXPECT_TRUE(cursor.isEof());
}


