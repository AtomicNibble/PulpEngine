#include "stdafx.h"

X_USING_NAMESPACE;

using namespace core;

#define EXPECT_NEAR_VEC3(expected, actual, angle_error) \
    {                                                   \
        Vec3f _exp = expected;                          \
        Vec3f _act = actual;                            \
        EXPECT_NEAR(_exp.x, _act.x, angle_error);       \
        EXPECT_NEAR(_exp.y, _act.y, angle_error);       \
        EXPECT_NEAR(_exp.z, _act.z, angle_error);       \
    }

TEST(SphereTest, Intersect)
{
    Vec3f center(0, 0, 0);
    float radius = 1.56f;

    Sphere sphere(center, radius);

    EXPECT_NEAR_VEC3(center, sphere.center(), 0.00000001f);
    EXPECT_NEAR(1.56f, sphere.radius(), 0.00000001f);

    Vec3f Origin(10, 0, 0);
    Vec3f Direction(-10, 0, 0);
    Ray ray(Origin, Direction);

    EXPECT_TRUE(sphere.intersects(ray));

    float intersection;
    EXPECT_TRUE(sphere.intersect(ray, &intersection));
    EXPECT_NEAR(intersection, 0.8439998f, 0.000001f);

    ray.setDirection(Vec3f(20, 20, 20));

    EXPECT_FALSE(sphere.intersects(ray));
}
