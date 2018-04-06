#include "stdafx.h"

#include <Util\PointerUtil.h>
#include <Util\PointerFlags.h>

//#include <Util\PointerWithFlags.h>

X_USING_NAMESPACE;

using namespace core;

TEST(PointerUtil, Align)
{
    void* unAlinPointer = reinterpret_cast<void*>(2);

    void* AlignedTop = pointerUtil::AlignTop(unAlinPointer, 4);
    void* AlignedBot = pointerUtil::AlignBottom(unAlinPointer, 4);

    EXPECT_TRUE(reinterpret_cast<uintptr_t>(AlignedTop) == 4);
    EXPECT_TRUE(reinterpret_cast<uintptr_t>(AlignedBot) == 0);

    unAlinPointer = reinterpret_cast<void*>(73);

    AlignedTop = pointerUtil::AlignTop(unAlinPointer, 8);
    AlignedBot = pointerUtil::AlignBottom(unAlinPointer, 8);

    EXPECT_TRUE(reinterpret_cast<uintptr_t>(AlignedTop) == 80);
    EXPECT_TRUE(reinterpret_cast<uintptr_t>(AlignedBot) == 72);
}

TEST(PointerUtil, Flags)
{
    X_ALIGNED_SYMBOL(struct Info, 8)
    {
        float f;
        int i;
    };

    Info info;
    info.f = 0.54321f;
    info.i = 0x22446688;

    PointerFlags<Info, 3> flag(&info);

    EXPECT_FALSE(flag.IsBitSet<0>());
    EXPECT_FALSE(flag.IsBitSet<1>());
    EXPECT_FALSE(flag.IsBitSet<2>());

    flag.SetBit<1>();

    EXPECT_TRUE(flag.GetBits() == 2);

    // set all 3
    flag.SetBits(7);

    EXPECT_TRUE(flag.GetBits() == 7);

    flag.ClearBit<0>();

    EXPECT_TRUE(flag.GetBits() == 6);

    EXPECT_TRUE(flag->i == 0x22446688);
}