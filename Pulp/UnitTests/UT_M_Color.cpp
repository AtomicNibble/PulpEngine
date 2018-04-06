#include "stdafx.h"

#include <Math\XPair.h>

X_USING_NAMESPACE;

using namespace core;

TEST(Col, Equality)
{
    {
        Color8u col(255, 100, 100);
        Color8u col1(255, 100, 100, 255);
        Color8u col2(26, 100, 100, 255);

        EXPECT_EQ(col, col1);
        EXPECT_NE(col, col2);
    }
    {
        Colorf col(0.5f, 0.76f, 0.99f);
        Colorf col1(0.5f, 0.76f, 0.99f, 1.0f);
        Colorf col2(0.1f, 0.2f, 0.3f, 0.5f);

        EXPECT_EQ(col, col1);
        EXPECT_NE(col, col2);
    }
}

TEST(Col, Con)
{
    {
        Color8u block(0, 0, 0, 255);
        Color8u gray(20, 20, 20, 255);
        Color8u white(255, 255, 255, 255);
        Color8u zero(0, 0, 0, 0);

        EXPECT_EQ(white, Color8u::white());
        EXPECT_EQ(block, Color8u::black());
        EXPECT_EQ(gray, Color8u::gray(20));
        EXPECT_EQ(zero, Color8u::zero());
    }
    {
        Colorf block(0.f, 0.f, 0.f, 1.0f);
        Colorf gray(0.2f, 0.2f, 0.2f, 1.0f);
        Colorf white(1.0f, 1.0f, 1.0f, 1.0f);
        Colorf zero(0.f, 0.f, 0.f, 0.f);

        EXPECT_EQ(white, Colorf::white());
        EXPECT_EQ(block, Colorf::black());
        EXPECT_EQ(gray, Colorf::gray(0.2f));
        EXPECT_EQ(zero, Colorf::zero());
    }
}

TEST(Col, Hex)
{
    {
        Color8u green = Color8u::hex(0x00ff00);
        Color8u blue = Color8u::hex(0x0000ff);
        Color8u red = Color8u::hex(0xff0000);
        Color8u red50 = Color8u::hexA(0x7dff0000);

        EXPECT_EQ(Color8u(0, 255, 0), green);
        EXPECT_EQ(Color8u(0, 0, 255), blue);
        EXPECT_EQ(Color8u(255, 0, 0), red);
        EXPECT_EQ(Color8u(255, 0, 0, 125), red50);
    }
    {
        float percision = 0.0099f;

        Colorf green = Colorf::hex(0x00ff00);
        Colorf blue = Colorf::hex(0x0000ff);
        Colorf red = Colorf::hex(0xff0000);
        Colorf red50 = Colorf::hexA(0x7dff0000);

        EXPECT_EQ(Colorf(0.f, 1.f, 0.f), green);
        EXPECT_EQ(Colorf(0.f, 0.f, 1.f), blue);
        EXPECT_EQ(Colorf(1.f, 0.f, 0.f), red);

        Colorf col(1.f, 0.f, 0.f, 0.5f);
        EXPECT_NEAR(col.a, red50.a, percision);
    }
}

TEST(Col, Packing)
{
    Color8u col(255, 150, 75, 125);

    EXPECT_EQ(0xf2, col.asRGB332());
    EXPECT_EQ(0x7F94, col.asARGB4444());
    EXPECT_EQ(0x7E49, col.asRGB555());
    EXPECT_EQ(0xFCA9, col.asRGB565());
    EXPECT_EQ(0x4B96FF, col.asBGR888());
    EXPECT_EQ(0xFF964B, col.asRGB888());
    EXPECT_EQ(0x7D4B96FF, col.asABGR8888());
    EXPECT_EQ(0x7DFF964B, col.asARGB8888());

    // compile will shrink to smallest float.
    Colorf colf(1.0f, 0.5882352941176471f, 0.2941176470588235f, 0.5f);

    // these are the same as above which is expected.
    EXPECT_EQ(0xf2, colf.asRGB332());
    EXPECT_EQ(0x7F94, colf.asARGB4444());
    EXPECT_EQ(0x7E49, colf.asRGB555());
    EXPECT_EQ(0xFCA9, colf.asRGB565());
    EXPECT_EQ(0x4B96FF, colf.asBGR888());
    EXPECT_EQ(0xFF964B, colf.asRGB888());

    // the alpha is 2 off O>O
    EXPECT_EQ(0x7F4B96FF, colf.asABGR8888());
    EXPECT_EQ(0x7FFF964B, colf.asARGB8888());
}

TEST(Col, Lerp)
{
    Color8u col1(0, 150, 75, 125);
    Color8u col2(255, 200, 25, 255);

    Color8u out = col1.lerp(0.5, col2);

    EXPECT_EQ(Color8u(127, 175, 50, 190), out);
}
