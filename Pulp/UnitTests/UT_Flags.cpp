#include "stdafx.h"

// slap goat, just don't tickle it's throat.
// or you will be going home in a boat.
// i wrote it's address on a note.
// be gone!

#include <Util\FlagsMacros.h>

X_USING_NAMESPACE;

using namespace core;

X_DECLARE_FLAGS(TestF)
(VAL_1, VAL_2, VAL_3, VAL_4, VAL_5, VAL_6, VAL_7, VAL_8,
    VAL_9, VAL_10);

TEST(FlagTest, Genral)
{
    Flags<TestF> flag;

    EXPECT_FALSE(flag.IsAnySet());
    EXPECT_FALSE(flag.AreAllSet());

    EXPECT_FALSE(flag.ToInt());

    flag = Flags<TestF>(TestF::VAL_1 | TestF::VAL_2);

    EXPECT_TRUE(flag.IsSet(TestF::VAL_1));
    EXPECT_TRUE(flag.IsSet(TestF::VAL_2));

    EXPECT_FALSE(flag.IsSet(TestF::VAL_3));
    EXPECT_FALSE(flag.IsSet(TestF::VAL_10));

    EXPECT_TRUE(flag.IsAnySet());
    EXPECT_FALSE(flag.AreAllSet());

    // set them all o.o
    flag |= Flags<TestF>(
        TestF::VAL_3 | TestF::VAL_4 | TestF::VAL_5 | TestF::VAL_6 | TestF::VAL_7 | TestF::VAL_8 | TestF::VAL_9 | TestF::VAL_10);

    // all should be set now.
    EXPECT_TRUE(flag.AreAllSet());

    EXPECT_TRUE(flag.ToInt() == 1023);

    flag.Clear();

    EXPECT_FALSE(flag.IsAnySet());
}
