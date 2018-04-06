#include "stdafx.h"

X_USING_NAMESPACE;

using namespace core;

TEST(Ray, Area)
{
    Vec3f Origin(0, 0, 0);
    Vec3f Direction(10, 10, 10);

    Ray ray(Origin, Direction);

    EXPECT_EQ(Vec3f(5.f, 5.f, 5.f), ray.calcPosition(0.5f));
    EXPECT_EQ(Vec3f(7.5f, 7.5f, 7.5f), ray.calcPosition(0.75f));
    EXPECT_EQ(Vec3f(2.5f, 2.5f, 2.5f), ray.calcPosition(0.25f));

    ray.setDirection(Vec3f(100.f, 80.f, 50.f));

    EXPECT_EQ(Vec3f(50.f, 40.f, 25.f), ray.calcPosition(0.5f));
}
