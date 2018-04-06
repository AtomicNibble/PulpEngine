#include "stdafx.h"

#include <Memory\MemCursor.h>

X_USING_NAMESPACE;

using namespace core;

TEST(MemCusor, Seek)
{
    int pData[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    core::MemCursor cursor(pData, 10);

    cursor.seek<int>(5);

    EXPECT_EQ(cursor.get<int>(), 5);

    cursor.seek<int>(5);

    EXPECT_TRUE(cursor.isEof());
}
