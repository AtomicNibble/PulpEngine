#include "stdafx.h"

X_USING_NAMESPACE;

using namespace core;

typedef ::testing::Types<float, double> MyTypes;
TYPED_TEST_CASE(MathPlane, MyTypes);

template<typename T>
class MathPlane : public ::testing::Test
{
public:
};

#define EXPECT_NEAR_VEC3(expected, actual, angle_error) \
    {                                                   \
        Vec3<TypeParam> _exp = expected;                \
        Vec3<TypeParam> _act = actual;                  \
        EXPECT_NEAR(_exp.x, _act.x, angle_error);       \
        EXPECT_NEAR(_exp.y, _act.y, angle_error);       \
        EXPECT_NEAR(_exp.z, _act.z, angle_error);       \
    }

TYPED_TEST(MathPlane, FromPoints)
{
    Vec3<TypeParam> v1(20, 10, 10);
    Vec3<TypeParam> v2(20, 10, 20);
    Vec3<TypeParam> v3(20, 20, 20);

    Plane<TypeParam> p(v1, v2, v3);

    Vec3<TypeParam> point = p.getPoint();
    Vec3<TypeParam> normal = p.getNormal();
    TypeParam distance = p.getDistance();

    EXPECT_NEAR_VEC3(Vec3<TypeParam>(20, 0, 0), point, 0.0000001f);
    EXPECT_NEAR_VEC3(Vec3<TypeParam>(1, 0, 0), normal, 0.0000001f);

    EXPECT_NEAR(20, distance, 0.0000001f);
}

