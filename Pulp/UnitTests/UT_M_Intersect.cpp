#include "stdafx.h"

#include <Math\XVector.h>
#include "Math\XIntersect.h"

#define EXPECT_NEAR_VEC3(expected, actual, angle_error) \
    {                                                   \
        Vec3f _exp = expected;                          \
        Vec3f _act = actual;                            \
        EXPECT_NEAR(_exp.x, _act.x, angle_error);       \
        EXPECT_NEAR(_exp.y, _act.y, angle_error);       \
        EXPECT_NEAR(_exp.z, _act.z, angle_error);       \
    }

TEST(Intersect, Ray)
{
    Vec3<float> v1(20, 10, 10);
    Vec3<float> v2(20, 10, 20);
    Vec3<float> v3(20, 20, 20);

    Ray ray(Vec3f(0.f), Vec3f(-20.f, 5.f, 0.f));
    Planef plane(v1, v2, v3);

    Vec3f output;
    bool result;

    result = plane.rayIntersection(ray, output);

    EXPECT_TRUE(result);
    EXPECT_NEAR_VEC3(Vec3f(20.f, -5.f, 0.f), output, 0.00001f);

    output = Vec3f::zero();

    result = plane.rayIntersection(Ray(Vec3f(20.f), Vec3f(0, 0, -20)), output);

    EXPECT_FALSE(result);
}