#include "stdafx.h"

// slap goat, just don't tickle it's throat.
// or you will be going home in a boat.
// i wrote it's address on a note.
// be gone!

#include <Util\FlagsMacros.h>

X_USING_NAMESPACE;

using namespace core;

namespace
{
    X_DECLARE_FLAGS(TestF)
    (VAL_1, VAL_2, VAL_3, VAL_4, VAL_5, VAL_6, VAL_7, VAL_8,
        VAL_9, VAL_10);

    X_DECLARE_FLAGS64(TestF64)(
        VAL_1, VAL_2, VAL_3, VAL_4, VAL_5, VAL_6, VAL_7, VAL_8, VAL_9, VAL_10,
        VAL_11, VAL_12, VAL_13, VAL_14, VAL_15, VAL_16, VAL_17, VAL_18, VAL_19, VAL_20,
        VAL_21, VAL_22, VAL_23, VAL_24, VAL_25, VAL_26, VAL_27, VAL_28, VAL_29, VAL_30,
        VAL_31, VAL_32, VAL_33, VAL_34, VAL_35, VAL_36, VAL_37, VAL_38, VAL_39, VAL_40
    );

} // namespace


TEST(Flags, Misc)
{
    Flags<TestF> flag;

    static_assert(std::is_trivially_copyable_v<Flags<TestF>>, "Should be trivially copyable");

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

TEST(Flags64, Misc)
{
    static_assert(TestF64::VAL_1 == 1ull);
    static_assert(TestF64::VAL_39 == 1ull << 38);
    static_assert(TestF64::VAL_40 == 1ull << 39);

    Flags64<TestF64> flag;
    static_assert(std::is_trivially_copyable_v<Flags64<TestF64>>, "Should be trivially copyable");

    EXPECT_FALSE(flag.IsAnySet());
    EXPECT_FALSE(flag.AreAllSet());

    EXPECT_FALSE(flag.ToInt());

    flag = Flags64<TestF64>(TestF64::VAL_1 | TestF64::VAL_2 | TestF64::VAL_36);

    EXPECT_TRUE(flag.IsSet(TestF64::VAL_1));
    EXPECT_TRUE(flag.IsSet(TestF64::VAL_2));
    EXPECT_TRUE(flag.IsSet(TestF64::VAL_36));

    EXPECT_FALSE(flag.IsSet(TestF64::VAL_3));
    EXPECT_FALSE(flag.IsSet(TestF64::VAL_10));
    EXPECT_FALSE(flag.IsSet(TestF64::VAL_30));

    EXPECT_TRUE(flag.IsAnySet());
    EXPECT_FALSE(flag.AreAllSet());

    constexpr uint64_t allFlags = TestF64::VAL_1 | TestF64::VAL_2 | TestF64::VAL_3 | TestF64::VAL_4 | TestF64::VAL_5 | TestF64::VAL_6 | TestF64::VAL_7 | TestF64::VAL_8 | TestF64::VAL_9 | TestF64::VAL_10 |
        TestF64::VAL_11 | TestF64::VAL_12 | TestF64::VAL_13 | TestF64::VAL_14 | TestF64::VAL_15 | TestF64::VAL_16 | TestF64::VAL_17 | TestF64::VAL_18 | TestF64::VAL_19 | TestF64::VAL_20 |
        TestF64::VAL_21 | TestF64::VAL_22 | TestF64::VAL_23 | TestF64::VAL_24 | TestF64::VAL_25 | TestF64::VAL_26 | TestF64::VAL_27 | TestF64::VAL_28 | TestF64::VAL_29 | TestF64::VAL_30 |
        TestF64::VAL_31 | TestF64::VAL_32 | TestF64::VAL_33 | TestF64::VAL_34 | TestF64::VAL_35 | TestF64::VAL_36 | TestF64::VAL_37 | TestF64::VAL_38 | TestF64::VAL_39 | TestF64::VAL_40;

    // Set them all
    flag |= Flags64<TestF64>(allFlags);

    // all should be set now.
    EXPECT_TRUE(flag.AreAllSet());

    EXPECT_TRUE(flag.ToInt() == allFlags);

    Flags64<TestF64>::Description buf;
    EXPECT_STREQ(flag.ToString(buf), "VAL_1, VAL_2, VAL_3, VAL_4, VAL_5, VAL_6, VAL_7, VAL_8, VAL_9, VAL_10, VAL_11, VAL_12, VAL_13, VAL_14, VAL_15, VAL_16, VAL_17, VAL_18, VAL_19, VAL_20, VAL_21, VAL_22, VAL_23, VAL_24, VAL_25, VAL_26, VAL_27, VAL_28, VAL_29, VAL_30, VAL_31, VAL_32, VAL_33, VAL_34, VAL_35, VAL_36, VAL_37, VAL_38, VAL_39, VAL_40");

    flag.Clear();

    EXPECT_FALSE(flag.IsAnySet());
    EXPECT_STREQ(flag.ToString(buf), "");
}
