#include "stdafx.h"

#include <Math\XRect.h>

X_USING_NAMESPACE;

using namespace core;

typedef ::testing::Types<float, double, int> MyTypes;
TYPED_TEST_CASE(Rect, MyTypes);

template<typename T>
class Rect : public ::testing::Test
{
public:
};

TYPED_TEST(Rect, Area)
{
    RectT<TypeParam> rec(10, 10, 110, 210);

    EXPECT_EQ(100, rec.getWidth());
    EXPECT_EQ(200, rec.getHeight());

    EXPECT_EQ(200 * 100, rec.calcArea());

    RectT<TypeParam> r2(10, 10, 210, 110);

    EXPECT_EQ(2, r2.getAspectRatio());
}

TYPED_TEST(Rect, Scale)
{
    RectT<TypeParam> rec(10, 10, 110, 210);

    rec.scale(3);
    EXPECT_EQ(100 * 3, rec.getWidth());
    EXPECT_EQ(200 * 3, rec.getHeight());

    rec.scale(Vec2<TypeParam>(1, 2));
    EXPECT_EQ(100 * 3, rec.getWidth());
    EXPECT_EQ((200 * 3) * 2, rec.getHeight());

    RectT<TypeParam> r2(100, 100, 200, 200);

    r2.scaleCentered(5);
    EXPECT_EQ(500, r2.getWidth());
    EXPECT_EQ(500, r2.getHeight());

    EXPECT_EQ(Vec2<TypeParam>(-100, -100), r2.getUpperLeft());
}

TYPED_TEST(Rect, Offset)
{
    RectT<TypeParam> r1(100, 100, 200, 200);

    r1.offset(Vec2<TypeParam>(100, 20));

    EXPECT_EQ(100, r1.getWidth());
    EXPECT_EQ(100, r1.getHeight());

    EXPECT_EQ(Vec2<TypeParam>(200, 120), r1.getUpperLeft());
}

TYPED_TEST(Rect, Contains)
{
    RectT<TypeParam> r1(100, 100, 200, 200);
    RectT<TypeParam> r2(150, 150, 200, 200);
    RectT<TypeParam> r3(50, 150, 200, 200);
    RectT<TypeParam> r4(199, 199, 200, 200);

    EXPECT_TRUE(r1.contains(r1));
    EXPECT_TRUE(r1.contains(r2));
    EXPECT_FALSE(r1.contains(r3));
    EXPECT_TRUE(r1.contains(r4));

    EXPECT_TRUE(r1.contains(Vec2<TypeParam>(150, 200)));
    EXPECT_TRUE(r1.contains(Vec2<TypeParam>(100, 100)));
    EXPECT_FALSE(r1.contains(Vec2<TypeParam>(99, 100)));

    // test int against all types.
    EXPECT_TRUE(r1.contains(Vec2i(150, 200)));
    EXPECT_TRUE(r1.contains(Vec2i(100, 100)));
    EXPECT_FALSE(r1.contains(Vec2i(99, 100)));
}

TYPED_TEST(Rect, Intersects)
{
    RectT<TypeParam> r1(100, 100, 200, 200);
    RectT<TypeParam> r2(150, 150, 200, 200);
    RectT<TypeParam> r3(50, 150, 200, 200);
    RectT<TypeParam> r4(199, 199, 200, 200);
    RectT<TypeParam> r5(-150, 0, 99, 99);
    RectT<TypeParam> r6(500, 200, 501, 201);

    EXPECT_TRUE(r1.intersects(r1));
    EXPECT_TRUE(r1.intersects(r2));
    EXPECT_TRUE(r1.intersects(r3));
    EXPECT_TRUE(r1.intersects(r4));

    EXPECT_FALSE(r1.intersects(r5));
    EXPECT_FALSE(r1.intersects(r6));
}

TYPED_TEST(Rect, Distance)
{
    RectT<TypeParam> r1(100, 100, 200, 200);

    EXPECT_EQ(0, r1.distance(Vec2<TypeParam>(150, 200)));
    EXPECT_EQ(0, r1.distance(Vec2<TypeParam>(100, 100)));
    EXPECT_EQ(1, r1.distance(Vec2<TypeParam>(99, 100)));

    EXPECT_NEAR(141, r1.distance(Vec2<TypeParam>(0, 0)), 0.9f);
    EXPECT_NEAR(100, r1.distance(Vec2<TypeParam>(0, 100)), 0.9f);
    EXPECT_NEAR(50, r1.distance(Vec2<TypeParam>(50, 100)), 0.9f);
}

TYPED_TEST(Rect, DistanceSqrd)
{
    RectT<TypeParam> r1(100, 100, 200, 200);

    EXPECT_EQ(0, r1.distanceSquared(Vec2<TypeParam>(150, 200)));
    EXPECT_EQ(0, r1.distanceSquared(Vec2<TypeParam>(100, 100)));
    EXPECT_EQ(1, r1.distanceSquared(Vec2<TypeParam>(99, 100)));

    EXPECT_NEAR(20000, r1.distanceSquared(Vec2<TypeParam>(0, 0)), 0.9f);
    EXPECT_NEAR(10000, r1.distanceSquared(Vec2<TypeParam>(0, 100)), 0.9f);
    EXPECT_NEAR(2500, r1.distanceSquared(Vec2<TypeParam>(50, 100)), 0.9f);
}

TYPED_TEST(Rect, Closest)
{
    RectT<TypeParam> r1(100, 100, 200, 200);

    EXPECT_EQ(Vec2<TypeParam>(150, 200), r1.closestPoint(Vec2<TypeParam>(150, 200)));
    EXPECT_EQ(Vec2<TypeParam>(100, 100), r1.closestPoint(Vec2<TypeParam>(99, 100)));

    EXPECT_EQ(Vec2<TypeParam>(100, 101), r1.closestPoint(Vec2<TypeParam>(1, 101)));
    EXPECT_EQ(Vec2<TypeParam>(150, 200), r1.closestPoint(Vec2<TypeParam>(150, 300)));
}

TYPED_TEST(Rect, Include)
{
    RectT<TypeParam> r1(100, 100, 200, 200);
    RectT<TypeParam> r2(150, 150, 200, 200);
    RectT<TypeParam> r3(50, 150, 200, 200);

    RectT<TypeParam> r1orig(r1);
    r1.include(r2);
    // should still be the same.
    EXPECT_EQ(r1orig.getWidth(), r1.getWidth());
    EXPECT_EQ(r1orig.getHeight(), r1.getHeight());

    r1.include(r3);

    EXPECT_EQ(150, r1.getWidth());
    EXPECT_EQ(100, r1.getHeight());

    r1.include(Vec2<TypeParam>(300, 300));

    EXPECT_EQ(250, r1.getWidth());
    EXPECT_EQ(200, r1.getHeight());
    EXPECT_EQ(50, r1.getX1());
    EXPECT_EQ(100, r1.getY1());
}

TYPED_TEST(Rect, Align)
{
    RectT<TypeParam> r1(100, 100, 200, 200);
    RectT<TypeParam> r2(500, 100, 600, 200);
    RectT<TypeParam> out;

    r2.Align(r1, Alignment::RIGHT_DOCK);

    EXPECT_EQ(r1.getX2(), r2.getX1());
    // TODO: add tests for other align types.
}

TYPED_TEST(Rect, Get)
{
    RectT<TypeParam> r1(100, 100, 200, 200);
    RectT<TypeParam> r2(500, 100, 700, 300);
    RectT<TypeParam> out;

    EXPECT_EQ(100, r1.getX1());
    EXPECT_EQ(200, r1.getX2());
    EXPECT_EQ(100, r1.getY1());
    EXPECT_EQ(200, r1.getY2());

    EXPECT_EQ(Vec2<TypeParam>(100, 100), r1.getUpperLeft());
    EXPECT_EQ(Vec2<TypeParam>(200, 100), r1.getUpperRight());
    EXPECT_EQ(Vec2<TypeParam>(200, 200), r1.getLowerRight());
    EXPECT_EQ(Vec2<TypeParam>(100, 200), r1.getLowerLeft());
    EXPECT_EQ(Vec2<TypeParam>(150, 150), r1.getCenter());
    EXPECT_EQ(Vec2<TypeParam>(100, 100), r1.getSize());

    out = r1.getCenteredFit(r2, false);
    EXPECT_EQ(Vec2<TypeParam>(550, 150), out.getUpperLeft());
    EXPECT_EQ(Vec2<TypeParam>(650, 250), out.getLowerRight());
}

TYPED_TEST(Rect, Arithmetic)
{
    RectT<TypeParam> r1(0, 0, 100, 100);

    r1 += Vec2<TypeParam>(100, 100); // offsets it.
    r1 -= Vec2<TypeParam>(50, 100);
    r1 *= 4;
    //	r1 /= 2;

    EXPECT_EQ(Vec2<TypeParam>(200, 0), r1.getUpperLeft());
    EXPECT_EQ(Vec2<TypeParam>(600, 400), r1.getLowerRight());
    EXPECT_EQ(Vec2<TypeParam>(400, 400), r1.getSize());
}
