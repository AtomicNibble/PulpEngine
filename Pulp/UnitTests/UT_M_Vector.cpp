#include "stdafx.h"

X_USING_NAMESPACE;

using namespace core;

typedef ::testing::Types<int16_t, uint16_t,
    int32_t, uint32_t,
    int64_t, uint64_t,
    float, double>
    MyTypes;

TYPED_TEST_CASE(TypedVector, MyTypes);

template<typename T>
class TypedVector : public ::testing::Test
{
public:
};

typedef ::testing::Types<int16_t,
    int32_t,
    int64_t,
    float, double>
    MyTypesSigned;

TYPED_TEST_CASE(TypedVectorSigned, MyTypesSigned);

template<typename T>
class TypedVectorSigned : public ::testing::Test
{
public:
};

#define EXPECT_NEAR_VEC2(expected, actual, angle_error) \
    {                                                   \
        Vec2f _exp = expected;                          \
        Vec2f _act = actual;                            \
        EXPECT_NEAR(_exp.x, _act.x, angle_error);       \
        EXPECT_NEAR(_exp.y, _act.y, angle_error);       \
    }

#define EXPECT_NEAR_VEC3(expected, actual, angle_error) \
    {                                                   \
        Vec3f _exp = expected;                          \
        Vec3f _act = actual;                            \
        EXPECT_NEAR(_exp.x, _act.x, angle_error);       \
        EXPECT_NEAR(_exp.y, _act.y, angle_error);       \
        EXPECT_NEAR(_exp.z, _act.z, angle_error);       \
    }

#define EXPECT_NEAR_VEC4(expected, actual, angle_error) \
    {                                                   \
        Vec4f _exp = expected;                          \
        Vec4f _act = actual;                            \
        EXPECT_NEAR(_exp.x, _act.x, angle_error);       \
        EXPECT_NEAR(_exp.y, _act.y, angle_error);       \
        EXPECT_NEAR(_exp.z, _act.z, angle_error);       \
        EXPECT_NEAR(_exp.w, _act.w, angle_error);       \
    }

TEST(XVector, Equality)
{
    Vec2f a2(1.f, 2.f);
    Vec2f b2(0.f, 2.f);
    Vec2f c2(1.f, 0.f);

    EXPECT_NE(a2, b2);
    EXPECT_NE(a2, c2);
    EXPECT_NE(b2, c2);

    EXPECT_EQ(a2, a2);
    EXPECT_EQ(b2, b2);

    Vec3f a3(1.f, 2.f, 3.f);
    Vec3f b3(0.f, 2.f, 3.f);
    Vec3f c3(1.f, 0.f, 3.f);
    Vec3f d3(1.f, 2.f, 0.f);

    EXPECT_NE(a3, b3);
    EXPECT_NE(a3, c3);
    EXPECT_NE(a3, d3);

    EXPECT_EQ(a3, a3);
    EXPECT_EQ(b3, b3);

    Vec4f a4(1.f, 2.f, 3.f, 4.f);
    Vec4f b4(0.f, 2.f, 3.f, 4.f);
    Vec4f c4(1.f, 0.f, 3.f, 4.f);
    Vec4f d4(1.f, 2.f, 0.f, 4.f);
    Vec4f e4(1.f, 2.f, 3.f, 0.f);

    EXPECT_FALSE(a4 == b4);
    EXPECT_FALSE(a4 == c4);
    EXPECT_FALSE(a4 == d4);
    EXPECT_FALSE(a4 == e4);

    EXPECT_TRUE(a4 == a4);
    EXPECT_TRUE(d4 == d4);
}

TEST(XVector, UnaryMinus)
{
    Vec2f a2(1.f, 3.f);
    Vec2f b2(-1.f, -3.f);

    EXPECT_EQ(a2, -b2);
    EXPECT_EQ(-a2, b2);

    Vec3f a3(1.f, 3.f, 5.f);
    Vec3f b3(-1.f, -3.f, -5.f);

    EXPECT_EQ(a3, -b3);
    EXPECT_EQ(-a3, b3);

    Vec4f a4(1.f, 3.f, 5.f, 8.f);
    Vec4f b4(-1.f, -3.f, -5.f, -8.f);

    EXPECT_TRUE(a4 == -b4);
    EXPECT_TRUE(-a4 == b4);
}

TEST(XVector, Normalize)
{
    Vec2f a2(3.f, 1.f);
    Vec2f a2_norm(0.948683262f, 0.316227764f);

    a2.normalize();
    EXPECT_FLOAT_EQ(a2.x, a2_norm.x);
    EXPECT_FLOAT_EQ(a2.y, a2_norm.y);

    Vec3f a3(3.f, 1.f, 2.f);
    Vec3f a3_norm(0.8017836f, 0.26726123f, 0.5345224f);

    a3.normalize();
    EXPECT_FLOAT_EQ(a3.x, a3_norm.x);
    EXPECT_FLOAT_EQ(a3.y, a3_norm.y);
    EXPECT_FLOAT_EQ(a3.z, a3_norm.z);

    Vec4f a4(3.f, 1.f, 2.f, 4.f);
    Vec4f a4_norm(0.547722578f, 0.182574183f, 0.365148365f, 0.730296731f);

    a4.normalize();
    EXPECT_FLOAT_EQ(a4.x, a4_norm.x);
    EXPECT_FLOAT_EQ(a4.y, a4_norm.y);
    EXPECT_FLOAT_EQ(a4.z, a4_norm.z);
    EXPECT_FLOAT_EQ(a4.w, a4_norm.w);
}

TEST(XVector, Dot)
{
    Vec2i a2(4, 8);
    Vec2i b2(2, 2);

    EXPECT_EQ(24, a2.dot(b2));

    Vec3i a3(4, 8, 10);
    Vec3i b3(2, 2, 1);

    EXPECT_EQ(34, a3.dot(b3));

    Vec4i a4(4, 8, 25, 4);
    Vec4i b4(2, 2, 14, 4);

    EXPECT_EQ(390, a4.dot(b4));
}

TEST(XVector, Cross)
{
    Vec2i a2(4, 8);
    Vec2i b2(2, 2);

    EXPECT_EQ(-8, a2.cross(b2));

    Vec3i a3(4, 8, 10);
    Vec3i b3(2, 2, 1);

    EXPECT_EQ(Vec3i(-12, 16, -8), a3.cross(b3));

    Vec4i a4(4, 8, 25, 4);
    Vec4i b4(2, 2, 14, 4);

    EXPECT_TRUE(a4.cross(b4) == Vec4i(62, -6, -8, 0));
}

TEST(XVector, Length)
{
    Vec2i a2(4, 8);

    EXPECT_NEAR(a2.length(), 8.94427f, 0.0001f);

    Vec3i a3(4, 8, 10);

    EXPECT_NEAR(a3.length(), 13.4164f, 0.0001f);

    Vec4i a4(4, 8, 25, 4);

    EXPECT_NEAR(a4.length(), 26.8514f, 0.0001f);
}

TEST(XVector, LengthSquared)
{
    Vec2i a2(4, 8);

    EXPECT_EQ(80, a2.lengthSquared());

    Vec3i a3(4, 8, 10);

    EXPECT_EQ(180, a3.lengthSquared());

    Vec4i a4(4, 8, 25, 4);

    EXPECT_EQ(721, a4.lengthSquared());
}

TEST(XVector, Distance)
{
    Vec2i a2(4, 8);
    Vec2i b2(40, 18);

    EXPECT_EQ(37, a2.distance(b2));

    Vec3i a3(4, 8, 10);
    Vec3i b3(12, 5, 1);

    EXPECT_EQ(12, a3.distance(b3));

    Vec4i a4(4, 8, 25, 4);
    Vec4i b4(1, 4, 5, 14);

    EXPECT_EQ(22, a4.distance(b4));
}

TEST(XVector, DistanceSquared)
{
    Vec2i a2(4, 8);
    Vec2i b2(40, 18);

    EXPECT_EQ(1396, a2.distanceSquared(b2));

    Vec3i a3(4, 8, 10);
    Vec3i b3(12, 5, 1);

    EXPECT_EQ(154, a3.distanceSquared(b3));

    Vec4i a4(4, 8, 25, 4);
    Vec4i b4(1, 4, 5, 14);

    EXPECT_EQ(525, a4.distanceSquared(b4));
}

TEST(XVector, Arithmic)
{
    // test some basic booty shaking.

    // addition.
    EXPECT_EQ(Vec2i(5, 5), Vec2i(2, 3) + Vec2i(3, 2));
    EXPECT_EQ(Vec3i(5, 5, 10), Vec3i(1, 4, 1) + Vec3i(4, 1, 9));
    EXPECT_TRUE(Vec4i(5, 5, 10, 15) == (Vec4i(2, 3, 5, 7) + Vec4i(3, 2, 5, 8)));

    // subtraction.
    EXPECT_EQ(Vec2i(-1, 1), Vec2i(2, 3) - Vec2i(3, 2));
    EXPECT_EQ(Vec3i(-3, 3, -8), Vec3i(1, 4, 1) - Vec3i(4, 1, 9));
    EXPECT_TRUE(Vec4i(-1, 1, 0, -1) == (Vec4i(2, 3, 5, 7) - Vec4i(3, 2, 5, 8)));

    // multiplication.
    //	EXPECT_EQ(Vec2i(6, 6), Vec2i(2, 3) * Vec2i(3, 2));
    //	EXPECT_EQ(Vec3i(4, 4, 9), Vec3i(1, 4, 1) * Vec3i(4, 1, 9));
    //	EXPECT_TRUE(Vec4i(6, 6, 25, 56) == (Vec4i(2, 3, 5, 7) * Vec4i(3, 2, 5, 8)));

    // division.
    EXPECT_EQ(Vec2i(0, 1), Vec2i(2, 3) / Vec2i(3, 2));
    EXPECT_EQ(Vec3i(0, 4, 0), Vec3i(1, 4, 1) / Vec3i(4, 1, 9));
    EXPECT_TRUE(Vec4i(0, 1, 1, 0) == (Vec4i(2, 3, 5, 7) / Vec4i(3, 2, 5, 8)));

    // need to test assign operators.
    // we just need to add the '=' it will make the correct code be used.

    // addition.
    EXPECT_EQ(Vec2i(5, 5), Vec2i(2, 3) += Vec2i(3, 2));
    EXPECT_EQ(Vec3i(5, 5, 10), Vec3i(1, 4, 1) += Vec3i(4, 1, 9));
    EXPECT_TRUE(Vec4i(5, 5, 10, 15) == (Vec4i(2, 3, 5, 7) += Vec4i(3, 2, 5, 8)));

    // subtraction.
    EXPECT_EQ(Vec2i(-1, 1), Vec2i(2, 3) -= Vec2i(3, 2));
    EXPECT_EQ(Vec3i(-3, 3, -8), Vec3i(1, 4, 1) -= Vec3i(4, 1, 9));
    EXPECT_TRUE(Vec4i(-1, 1, 0, -1) == (Vec4i(2, 3, 5, 7) -= Vec4i(3, 2, 5, 8)));

    // multiplication.
    EXPECT_EQ(Vec2i(6, 6), Vec2i(2, 3) *= Vec2i(3, 2));
    EXPECT_EQ(Vec3i(4, 4, 9), Vec3i(1, 4, 1) *= Vec3i(4, 1, 9));
    EXPECT_TRUE(Vec4i(6, 6, 25, 56) == (Vec4i(2, 3, 5, 7) *= Vec4i(3, 2, 5, 8)));

    // division.
    EXPECT_EQ(Vec2i(0, 1), Vec2i(2, 3) /= Vec2i(3, 2));
    EXPECT_EQ(Vec3i(0, 4, 0), Vec3i(1, 4, 1) /= Vec3i(4, 1, 9));
    EXPECT_TRUE(Vec4i(0, 1, 1, 0) == (Vec4i(2, 3, 5, 7) /= Vec4i(3, 2, 5, 8)));
}

TEST(XVector, Orthogonal)
{
    // only vec3 supports this.
    Vec3f a3(5, 6, 7);
    Vec3f b3(5, 0, 7);
    Vec3f c3(2, 6, 3);

    Vec3f res = c3.getOrthogonal();

    EXPECT_EQ(Vec3f(0, 7, -6), a3.getOrthogonal());
    EXPECT_EQ(Vec3f(-7, 0, 5), b3.getOrthogonal());
    EXPECT_EQ(Vec3f(0, 3, -6), c3.getOrthogonal());
}

TEST(XVector, Rotate)
{
    // only vec3 supports this.
    Vec3f a3(50, 60, 70);

    a3.rotateX(24);

    EXPECT_NEAR(50.f, a3.x, 0.0001f);
    EXPECT_NEAR(88.8412323f, a3.y, 0.0001f);
    EXPECT_NEAR(-24.6421700f, a3.z, 0.0001f);

    a3.rotateY(59);

    EXPECT_NEAR(-22.8634033f, a3.x, 0.0001f);
    EXPECT_NEAR(88.8412323f, a3.y, 0.0001f);
    EXPECT_NEAR(50.8379898f, a3.z, 0.0001f);

    a3.rotateZ(127);

    EXPECT_NEAR(-91.7221756f, a3.x, 0.0001f);
    EXPECT_NEAR(-1.59456635f, a3.y, 0.0001f);
    EXPECT_NEAR(50.8379898f, a3.z, 0.0001f);

    a3.rotate(Vec3f(0.345f, 0.987f, 0.9753f), 45);
    EXPECT_NEAR(-1.48461151f, a3.x, 0.0001f);
    EXPECT_NEAR(-84.2139206f, a3.y, 0.0001f);
    EXPECT_NEAR(110.846237f, a3.z, 0.0001f);
}

TEST(XVector, Lerp)
{
    Vec2f a2(0.1f, 0.2f);
    Vec2f b2(0.2f, 0.8f);

    EXPECT_NEAR_VEC2(Vec2f(0.15f, 0.5f), a2.lerp(0.5f, b2), 0.00001f);
    EXPECT_NEAR_VEC2(Vec2f(0.155f, 0.53f), a2.lerp(0.55f, b2), 0.00001f);

    Vec3f a3(0.1f, 0.2f, 0.4f);
    Vec3f b3(0.2f, 0.8f, 0.8f);

    EXPECT_NEAR_VEC3(Vec3f(0.15f, 0.5f, 0.6f), a3.lerp(0.5f, b3), 0.00001f);
    EXPECT_NEAR_VEC3(Vec3f(0.155f, 0.53f, 0.62f), a3.lerp(0.55f, b3), 0.00001f);
    EXPECT_NEAR_VEC3(Vec3f(0.100001f, 0.20006f, 0.40004f), a3.lerp(0.0001f, b3), 0.00001f);
    EXPECT_NEAR_VEC3(Vec3f(0.1990f, 0.7940f, 0.7960f), a3.lerp(0.99f, b3), 0.00001f);

    Vec4f a4(0.1f, 0.2f, 0.4f, 0.f);
    Vec4f b4(0.2f, 0.8f, 0.8f, 1.f);

    EXPECT_NEAR_VEC4(Vec4f(0.15f, 0.5f, 0.6f, 0.5f), a4.lerp(0.5f, b4), 0.00001f);
    EXPECT_NEAR_VEC4(Vec4f(0.155f, 0.53f, 0.62f, 0.55f), a4.lerp(0.55f, b4), 0.00001f);
}

TEST(XVector, Slerp)
{
    // spherical linear interpolation
    // Let p0 and p1 be the first and last points of the arc,
    // and let t be the parameter, 0 < t < 1.
    // Compute F as the angle subtended by the arc

    Vec3f a3(0.1f, 0.2f, 0.4f);
    Vec3f b3(0.2f, 0.8f, 0.8f);

    EXPECT_NEAR_VEC3(Vec3f(0.173205078f, 0.577350259f, 0.692820311f), a3.slerp(0.5f, b3), 0.00001f);
    EXPECT_NEAR_VEC3(Vec3f(0.178201318f, 0.607960641f, 0.712805271f), a3.slerp(0.55f, b3), 0.00001f);
    EXPECT_NEAR_VEC3(Vec3f(0.100018129f, 0.200084627f, 0.400072515f), a3.slerp(0.0001f, b3), 0.00001f);
    EXPECT_NEAR_VEC3(Vec3f(0.199989021f, 0.797537744f, 0.799956083f), a3.slerp(0.99f, b3), 0.00001f);

    Vec4f a4(0.1f, 0.2f, 0.4f, 0.f);

    EXPECT_NEAR_VEC4(Vec4f(0.178201318f, 0.607960641f, 0.712805271f, 0.000000000), a4.slerp(0.55f, b3), 0.00001f);
    EXPECT_NEAR_VEC4(Vec4f(0.100018129f, 0.200084627f, 0.400072515f, 0.000000000), a4.slerp(0.0001f, b3), 0.00001f);
}

TEST(XVector, Limit)
{
    Vec2f a2(0.1f, 0.2f);
    EXPECT_NEAR_VEC2(a2, a2.limited(.5f), 0.00001f);
    EXPECT_NEAR_VEC2(Vec2f(0.0894427225f, 0.178885445f), a2.limited(.2f), 0.00001f);
    // check the other function.
    a2.limit(.2f);
    EXPECT_NEAR_VEC2(Vec2f(0.0894427225f, 0.178885445f), a2, 0.00001f);

    Vec3f a3(0.1f, 0.2f, 0.4f);
    EXPECT_NEAR_VEC3(a3, a3.limited(.5f), 0.00001f);
    EXPECT_NEAR_VEC3(Vec3f(0.0436435752f, 0.0872871503f, 0.174574301f), a3.limited(.2f), 0.00001f);
    // check the other function.
    a3.limit(.2f);
    EXPECT_NEAR_VEC3(Vec3f(0.0436435752f, 0.0872871503f, 0.174574301f), a3, 0.00001f);

    Vec4f a4(0.2f, 0.7f, 0.4f, 0.3f);
    EXPECT_NEAR_VEC4(a4, a4.limited(.99f), 0.00001f);
    EXPECT_NEAR_VEC4(Vec4f(0.0452910811f, 0.158518776f, 0.0905821621f, 0.0679366216f), a4.limited(.2f), 0.00001f);
    // check the other function.
    a4.limit(.2f);
    EXPECT_NEAR_VEC4(Vec4f(0.0452910811f, 0.158518776f, 0.0905821621f, 0.0679366216f), a4, 0.00001f);
}

TYPED_TEST(TypedVector, Min)
{
    typedef Vec2<TypeParam> Vec2T;
    typedef Vec3<TypeParam> Vec3T;
    typedef Vec4<TypeParam> Vec4T;
    typedef Vec5<TypeParam> Vec5T;

    Vec2T min2 = Vec2T::min();
    EXPECT_EQ(std::numeric_limits<TypeParam>::lowest(), min2.x);
    EXPECT_EQ(std::numeric_limits<TypeParam>::lowest(), min2.y);

    Vec3T min3 = Vec3T::min();
    EXPECT_EQ(std::numeric_limits<TypeParam>::lowest(), min3.x);
    EXPECT_EQ(std::numeric_limits<TypeParam>::lowest(), min3.y);
    EXPECT_EQ(std::numeric_limits<TypeParam>::lowest(), min3.z);

    Vec4T min4 = Vec4T::min();
    EXPECT_EQ(std::numeric_limits<TypeParam>::lowest(), min4.x);
    EXPECT_EQ(std::numeric_limits<TypeParam>::lowest(), min4.y);
    EXPECT_EQ(std::numeric_limits<TypeParam>::lowest(), min4.z);
    EXPECT_EQ(std::numeric_limits<TypeParam>::lowest(), min4.w);

    Vec5T min5 = Vec5T::min();
    EXPECT_EQ(std::numeric_limits<TypeParam>::lowest(), min5.x);
    EXPECT_EQ(std::numeric_limits<TypeParam>::lowest(), min5.y);
    EXPECT_EQ(std::numeric_limits<TypeParam>::lowest(), min5.z);
    EXPECT_EQ(std::numeric_limits<TypeParam>::lowest(), min5.s);
    EXPECT_EQ(std::numeric_limits<TypeParam>::lowest(), min5.t);
}

TYPED_TEST(TypedVector, Max)
{
    typedef Vec2<TypeParam> Vec2T;
    typedef Vec3<TypeParam> Vec3T;
    typedef Vec4<TypeParam> Vec4T;
    typedef Vec5<TypeParam> Vec5T;

    Vec2T min2 = Vec2T::max();
    EXPECT_EQ(std::numeric_limits<TypeParam>::max(), min2.x);
    EXPECT_EQ(std::numeric_limits<TypeParam>::max(), min2.y);

    Vec3T min3 = Vec3T::max();
    EXPECT_EQ(std::numeric_limits<TypeParam>::max(), min3.x);
    EXPECT_EQ(std::numeric_limits<TypeParam>::max(), min3.y);
    EXPECT_EQ(std::numeric_limits<TypeParam>::max(), min3.z);

    Vec4T min4 = Vec4T::max();
    EXPECT_EQ(std::numeric_limits<TypeParam>::max(), min4.x);
    EXPECT_EQ(std::numeric_limits<TypeParam>::max(), min4.y);
    EXPECT_EQ(std::numeric_limits<TypeParam>::max(), min4.z);
    EXPECT_EQ(std::numeric_limits<TypeParam>::max(), min4.w);

    Vec5T min5 = Vec5T::max();
    EXPECT_EQ(std::numeric_limits<TypeParam>::max(), min5.x);
    EXPECT_EQ(std::numeric_limits<TypeParam>::max(), min5.y);
    EXPECT_EQ(std::numeric_limits<TypeParam>::max(), min5.z);
    EXPECT_EQ(std::numeric_limits<TypeParam>::max(), min5.s);
    EXPECT_EQ(std::numeric_limits<TypeParam>::max(), min5.t);
}

TYPED_TEST(TypedVectorSigned, abs)
{
    typedef Vec2<TypeParam> Vec2T;
    typedef Vec3<TypeParam> Vec3T;
    typedef Vec4<TypeParam> Vec4T;
    typedef Vec5<TypeParam> Vec5T;
    typedef TypeParam T;

    Vec2T abs2 = Vec2T(static_cast<T>(15.2),
        static_cast<T>(-15))
                     .abs();

    EXPECT_EQ(static_cast<T>(15.2), abs2.x);
    EXPECT_EQ(static_cast<T>(15), abs2.y);

    Vec3T abs3 = Vec3T(static_cast<T>(15.2),
        static_cast<T>(-15),
        static_cast<T>(15))
                     .abs();

    EXPECT_EQ(static_cast<T>(15.2), abs3.x);
    EXPECT_EQ(static_cast<T>(15), abs3.y);
    EXPECT_EQ(static_cast<T>(15), abs3.z);

    X_DISABLE_WARNING(4309)
    Vec4T abs4 = Vec4T(static_cast<T>(15.2),
        static_cast<T>(-15),
        static_cast<T>(15),
        static_cast<T>(-4.94258))
                     .abs();
    X_ENABLE_WARNING(4309)

    EXPECT_EQ(static_cast<T>(15.2), abs4.x);
    EXPECT_EQ(static_cast<T>(15), abs4.y);
    EXPECT_EQ(static_cast<T>(15), abs4.z);
    EXPECT_EQ(static_cast<T>(4.94258), abs4.w);

    X_DISABLE_WARNING(4309)
    Vec5T abs5 = Vec5T(static_cast<T>(15.2),
        static_cast<T>(-15),
        static_cast<T>(15),
        static_cast<T>(-4.94258),
        static_cast<T>(-0.001))
                     .abs();
    X_ENABLE_WARNING(4309)

    EXPECT_EQ(static_cast<T>(15.2), abs5.x);
    EXPECT_EQ(static_cast<T>(15), abs5.y);
    EXPECT_EQ(static_cast<T>(15), abs5.z);
    EXPECT_EQ(static_cast<T>(4.94258), abs5.s);
    EXPECT_EQ(static_cast<T>(0.001), abs5.t);
}
