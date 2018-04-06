#include "stdafx.h"

#include <Math\XQuatCompressed.h>

#define EXPECT_NEAR_VEC3(expected, actual, angle_error) \
    {                                                   \
        Vec3<TypeParam> _exp = expected;                \
        Vec3<TypeParam> _act = actual;                  \
        EXPECT_NEAR(_exp.x, _act.x, angle_error);       \
        EXPECT_NEAR(_exp.y, _act.y, angle_error);       \
        EXPECT_NEAR(_exp.z, _act.z, angle_error);       \
    }

#define EXPECT_NEAR_QUAT(expected, actual, angle_error)  \
    EXPECT_NEAR_VEC3(expected.v, actual.v, angle_error); \
    EXPECT_NEAR(expected.w, actual.w, angle_error);

X_USING_NAMESPACE;

using namespace core;

typedef ::testing::Types<float, double> MyTypes;
TYPED_TEST_CASE(QuatTest, MyTypes);

template<typename T>
class QuatTest : public ::testing::Test
{
public:
};

TYPED_TEST(QuatTest, Comp)
{
    Quat<TypeParam> quat(-0.5f, -0.112345f, 0.76543f, 0.41234f);

    XQuatCompressed<TypeParam> compressed(quat);

    Quat<TypeParam> out = compressed.asQuat();

    EXPECT_NEAR_QUAT(quat, out, 0.0001f);
}