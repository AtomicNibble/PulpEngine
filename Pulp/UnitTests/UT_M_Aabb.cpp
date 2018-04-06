#include "stdafx.h"

#include <Math\XAabb.h>

// streames not used in engine, so declare.
::std::ostream& operator<<(::std::ostream& os, const Vec3f& bar)
{
    return os << "(" << bar.x << ", " << bar.y << ", " << bar.z << ")";
}

TEST(AAbb, Constructor)
{
    AABB aabb;

    EXPECT_TRUE(aabb.isEmpty());
    EXPECT_FLOAT_EQ(0.f, aabb.radius());
    EXPECT_FLOAT_EQ(0.f, aabb.radiusSqr());
    EXPECT_FLOAT_EQ(0.f, aabb.volume());
    EXPECT_EQ(Vec3f(0.f), aabb.size());
    EXPECT_EQ(Vec3f(0.f), aabb.center());
}

TEST(AAbb, Constructor1)
{
    AABB aabb(5.f);

    EXPECT_FALSE(aabb.isEmpty());
    EXPECT_NEAR(8.66025f, aabb.radius(), 0.0001f);
    EXPECT_FLOAT_EQ(75.f, aabb.radiusSqr());
    EXPECT_FLOAT_EQ(1000.f, aabb.volume());
    EXPECT_EQ(Vec3f(-10.f), aabb.size());
    EXPECT_EQ(Vec3f(0.f), aabb.center());
}

TEST(AAbb, Constructor2)
{
    AABB aabb(Vec3f(5.f), Vec3f(10.f));

    EXPECT_FALSE(aabb.isEmpty());
    EXPECT_NEAR(4.3301f, aabb.radius(), 0.0001f);
    EXPECT_FLOAT_EQ(18.75f, aabb.radiusSqr());
    EXPECT_FLOAT_EQ(-125, aabb.volume());
    EXPECT_EQ(Vec3f(5.f), aabb.size());
    EXPECT_EQ(Vec3f(7.5f), aabb.center());
}

TEST(AAbb, Constructor3)
{
    AABB aabb__(Vec3f(5.f), Vec3f(10.f));
    AABB aabb(aabb__);

    EXPECT_FALSE(aabb.isEmpty());
    EXPECT_NEAR(4.3301f, aabb.radius(), 0.0001f);
    EXPECT_FLOAT_EQ(18.75f, aabb.radiusSqr());
    EXPECT_FLOAT_EQ(-125, aabb.volume());
    EXPECT_EQ(Vec3f(5.f), aabb.size());
    EXPECT_EQ(Vec3f(7.5f), aabb.center());
}

TEST(AAbb, Set)
{
    AABB aabb__(Vec3f(5.f), Vec3f(10.f));
    AABB aabb;

    aabb.set(5.f);
    EXPECT_FALSE(aabb.isEmpty());
    EXPECT_NEAR(8.66025f, aabb.radius(), 0.0001f);
    EXPECT_FLOAT_EQ(75.f, aabb.radiusSqr());
    EXPECT_FLOAT_EQ(1000.f, aabb.volume());
    EXPECT_EQ(Vec3f(-10.f), aabb.size());
    EXPECT_EQ(Vec3f(0.f), aabb.center());

    aabb.set(Vec3f(5.f), Vec3f(10.f));
    EXPECT_FALSE(aabb.isEmpty());
    EXPECT_NEAR(4.3301f, aabb.radius(), 0.0001f);
    EXPECT_FLOAT_EQ(18.75f, aabb.radiusSqr());
    EXPECT_FLOAT_EQ(-125, aabb.volume());
    EXPECT_EQ(Vec3f(5.f), aabb.size());
    EXPECT_EQ(Vec3f(7.5f), aabb.center());

    aabb.set(aabb__);
    EXPECT_FALSE(aabb.isEmpty());
    EXPECT_NEAR(4.3301f, aabb.radius(), 0.0001f);
    EXPECT_FLOAT_EQ(18.75f, aabb.radiusSqr());
    EXPECT_FLOAT_EQ(-125, aabb.volume());
    EXPECT_EQ(Vec3f(5.f), aabb.size());
    EXPECT_EQ(Vec3f(7.5f), aabb.center());
}

TEST(AAbb, Add)
{
    AABB aabb(Vec3f(5.f), Vec3f(10.f));

    aabb.add(Vec3f(15)); // make it: 5,5,5 : 15,15,15,
    EXPECT_FALSE(aabb.isEmpty());
    EXPECT_NEAR(8.66025f, aabb.radius(), 0.0001f);
    EXPECT_FLOAT_EQ(75.f, aabb.radiusSqr());
    EXPECT_FLOAT_EQ(-1000.f, aabb.volume());
    EXPECT_EQ(Vec3f(10.f), aabb.size());
    EXPECT_EQ(Vec3f(10.f), aabb.center());

    aabb.add(Vec3f(3.f, 4.f, 5.f), 1.f); // make it: 2,3,4 : 15,15,15,
    EXPECT_FALSE(aabb.isEmpty());
    EXPECT_NEAR(10.416333f, aabb.radius(), 0.0001f);
    EXPECT_FLOAT_EQ(108.5f, aabb.radiusSqr());
    EXPECT_FLOAT_EQ(-1716.f, aabb.volume());
    EXPECT_EQ(Vec3f(13.f, 12.f, 11.f), aabb.size());
    EXPECT_EQ(Vec3f(8.5f, 9.f, 9.5f), aabb.center());

    aabb.add(AABB(Vec3f(0.f), Vec3f(20.f))); // make it: 0,0,0 : 20,20,20 cus i'm lazy (been lazy in a unit test == ~baller)
    EXPECT_FALSE(aabb.isEmpty());
    EXPECT_NEAR(17.320508f, aabb.radius(), 0.0001f);
    EXPECT_FLOAT_EQ(300.f, aabb.radiusSqr());
    EXPECT_FLOAT_EQ(-8000.f, aabb.volume());
    EXPECT_EQ(Vec3f(20.f, 20.f, 20.f), aabb.size());
    EXPECT_EQ(Vec3f(10.f, 10.f, 10.f), aabb.center());
}

TEST(AAbb, Move)
{
    AABB aabb(Vec3f(5.f), Vec3f(10.f));

    // i like to
    aabb.move(Vec3f(25.f));
    //                it, move it!

    EXPECT_FALSE(aabb.isEmpty());
    EXPECT_NEAR(4.3301f, aabb.radius(), 0.0001f);
    EXPECT_FLOAT_EQ(18.75f, aabb.radiusSqr());
    EXPECT_FLOAT_EQ(-125, aabb.volume());
    EXPECT_EQ(Vec3f(5.f), aabb.size());
    EXPECT_EQ(Vec3f(32.5f), aabb.center());
}

TEST(AAbb, Expand)
{
    AABB aabb(Vec3f(5.f), Vec3f(10.f));

    aabb.expand(Vec3f(25.f));

    EXPECT_FALSE(aabb.isEmpty());
    EXPECT_NEAR(47.631397f, aabb.radius(), 0.0001f);
    EXPECT_FLOAT_EQ(2268.75f, aabb.radiusSqr());
    EXPECT_FLOAT_EQ(-166375, aabb.volume());
    EXPECT_EQ(Vec3f(55.f), aabb.size());
    EXPECT_EQ(Vec3f(7.5f), aabb.center());
}

TEST(AAbb, Clip)
{
    AABB aabb(Vec3f(5.f), Vec3f(10.f));

    aabb.clip(AABB(Vec3f(0.f), Vec3f(8.f))); // should give us: 5,5,5 : 8,8,8

    // just check these, we checked the others enougth.
    EXPECT_EQ(Vec3f(3.f), aabb.size());
    EXPECT_EQ(Vec3f(6.5f), aabb.center());
}

TEST(AAbb, Contains)
{
    AABB aabb(Vec3f(5.f), Vec3f(10.f, 16.f, 70.f));

    EXPECT_FALSE(aabb.containsPoint(Vec3f(-6.f)));
    EXPECT_FALSE(aabb.containsPoint(Vec3f(7.f, 8.f, 80.f)));
    EXPECT_FALSE(aabb.containsPoint(Vec3f(9.f, 20.f, 10.f)));
    EXPECT_TRUE(aabb.containsPoint(Vec3f(5.f)));
    EXPECT_TRUE(aabb.containsPoint(Vec3f(6.f, 15.f, 65.f)));

    EXPECT_FALSE(aabb.containsBox(AABB(Vec3f(0.f), Vec3f(8.f))));
    EXPECT_FALSE(aabb.containsBox(AABB(Vec3f(5.f), Vec3f(10.f, 16.f, 70.1f)))); // just too big...
    EXPECT_FALSE(aabb.containsBox(AABB(Vec3f(0.f), Vec3f(4.9f))));
    EXPECT_FALSE(aabb.containsBox(AABB(Vec3f(-6.f), Vec3f(-5.f))));
    EXPECT_TRUE(aabb.containsBox(AABB(Vec3f(5.f), Vec3f(10.f, 16.f, 69.9f)))); // only just fits. (that's what she said)
    EXPECT_TRUE(aabb.containsBox(aabb));                                       // should contain itself.
    EXPECT_TRUE(aabb.containsBox(AABB(Vec3f(7.f), Vec3f(6.f))));               // slides right in!

    // sorry for the comments :(

    EXPECT_FALSE(aabb.containsSphere(Sphere(Vec3f(6.f), 2.f)));
    EXPECT_TRUE(aabb.containsSphere(Sphere(Vec3f(8.f), 1.f)));

    EXPECT_FALSE(aabb.containsSphere(Vec3f(6.f), 2.f));
    EXPECT_TRUE(aabb.containsSphere(Vec3f(8.f), 1.f));
}

TEST(AAbb, Intersects)
{
    AABB aabb(Vec3f(5.f), Vec3f(10.f, 16.f, 70.f));

    EXPECT_TRUE(aabb.intersects(AABB(Vec3f(-5.f), Vec3f(6.f))));
    EXPECT_TRUE(aabb.intersects(AABB(Vec3f(10.f, 15.f, 15.f), Vec3f(20.f))));
    EXPECT_TRUE(aabb.intersects(AABB(Vec3f(5.f, 15.f, 70.f), Vec3f(95.f))));

    EXPECT_FALSE(aabb.intersects(AABB(Vec3f(-5.f), Vec3f(4.9f))));
    EXPECT_FALSE(aabb.intersects(AABB(Vec3f(15.f, 20.f, 80.f), Vec3f(100.f))));
}

TEST(AAbb, Distance)
{
    AABB aabb(Vec3f(5.f), Vec3f(10.f, 10.f, 10.f));

    EXPECT_FLOAT_EQ(86.602539f, aabb.distance(Vec3f(60.f)));
    EXPECT_FLOAT_EQ(7500.f, aabb.distanceSqr(Vec3f(60.f)));

    // some less generic ones
    aabb.set(Vec3f(5.f), Vec3f(10.f, 15.f, 13.f));
    EXPECT_FLOAT_EQ(82.060951f, aabb.distance(Vec3f(60.f)));
    EXPECT_FLOAT_EQ(6734.f, aabb.distanceSqr(Vec3f(60.f)));

    aabb.set(Vec3f(5.f, 0.f, 10.f), Vec3f(10.f, 15.f, 20.f));
    EXPECT_FLOAT_EQ(78.2624f, aabb.distance(Vec3f(60.f)));
    EXPECT_FLOAT_EQ(6125.f, aabb.distanceSqr(Vec3f(60.f)));
}
