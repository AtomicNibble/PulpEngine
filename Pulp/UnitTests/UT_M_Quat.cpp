#include "stdafx.h"

#define EXPECT_NEAR_VEC3(expected, actual, angle_error) \
    {                                                   \
        Vec3f _exp = expected;                          \
        Vec3f _act = actual;                            \
        EXPECT_NEAR(_exp.x, _act.x, angle_error);       \
        EXPECT_NEAR(_exp.y, _act.y, angle_error);       \
        EXPECT_NEAR(_exp.z, _act.z, angle_error);       \
    }

#define EXPECT_NEAR_QUAT(expected, actual, angle_error)  \
    EXPECT_NEAR_VEC3(expected.v, actual.v, angle_error); \
    EXPECT_NEAR(expected.w, actual.w, angle_error);

X_USING_NAMESPACE;

using namespace core;

TEST(XQuat, AxisAngle)
{
    const Vec3f axis(-0.0381526f, -0.9628002f, 0.2675074f);
    const float radians = 4.4348185f;

    Quatf q(axis, radians);

    // Check the Quat
    EXPECT_NEAR_QUAT(Quatf(-0.6024865f, -0.0304507f, -0.7684388f, 0.2135054f), q, 0.00001f);

    // Check Axis angles comes back the same.
    Vec3f axisOut;
    float radiansOut;
    q.getAxisAngle(&axisOut, &radiansOut);

    EXPECT_NEAR_VEC3(axisOut, axis, 0.00001f);
    EXPECT_NEAR(radians, radiansOut, 0.00001f);
}

TEST(XQuat, Axis)
{
    Quatf a0(0.49879f, 0.232f, 0.463f, 0.695f);

    EXPECT_NEAR(1.f, a0.length(), 0.00001f);

    float32_t pitch = a0.getPitch();
    float32_t yaw = a0.getYaw();
    float32_t roll = a0.getRoll();

    EXPECT_FLOAT_EQ(1.08355474f, pitch);
    EXPECT_FLOAT_EQ(0.139854997f, yaw);
    EXPECT_FLOAT_EQ(1.98085940f, roll);

    Vec3f vec = a0.getAxis();
    float angle = a0.getAngle();
    EXPECT_NEAR_VEC3(Vec3f(0.267674923f, 0.534196079f, 0.801871061f), vec, 0.00001f);
    EXPECT_NEAR(2.0971938, angle, 0.00001f);

    {
        Quatf q1(Vec3f(0.7571761f, 0.0101299f, 0.6531323f), ::toRadians(240.2215912f));
        EXPECT_NEAR(1.f, q1.length(), 0.00001f);
        EXPECT_NEAR_QUAT(Quatf(-0.5016737f, 0.6550004f, 0.008763f, 0.5649966f), q1, 0.00001f);

        Vec3f vec = q1.getAxis();
        float angle = q1.getAngle();
        Vec3f vecMangitude = vec * ::toDegrees(angle);
     
        EXPECT_NEAR_VEC3(Vec3f(0.7571761f, 0.0101299f, 0.6531323f), vec, 0.00001f);
        EXPECT_NEAR(4.1926577f, angle, 0.00001f);
        EXPECT_NEAR_VEC3(Vec3f(181.8900362f, 2.4334257f, 156.8964819f), vecMangitude, 0.0001f);
    }
}

TEST(XQuat, Matrix)
{
    const Quatf ident = Quatf::identity();
    EXPECT_EQ(ident.toMatrix33(), Matrix33f::identity());

    const Matrix33f m0(
        1.0000000f, 0.0000000f, 0.0000000f,
        0.0000000f, -0.4480736f, -0.8939967f,
        0.0000000f, 0.8939967f, -0.4480736f, true
    );

    Quatf q0(m0);
    EXPECT_NEAR(1.f, q0.length(), 0.00001f);
    EXPECT_NEAR_QUAT(Quatf(0.525321960f, 0.8509035f, 0.f, 0.f), q0, 0.00001f);
    EXPECT_EQ(m0, q0.toMatrix33());

    const Matrix33f m1(
        -0.4480736f, -0.0000000f, 0.8939967f,
        0.7992300f, -0.4480736f, 0.4005763f,
        0.4005763f, 0.8939967f, 0.2007700f, true
    );
    
    Quatf q1(m1);
    EXPECT_NEAR(1.f, q1.length(), 0.00001f);
    EXPECT_NEAR_QUAT(Quatf(0.2759632f, 0.4469983f, 0.4469983f, 0.7240368f), q1, 0.00001f);
    EXPECT_EQ(m1, q1.toMatrix33());

    const Matrix33f m2(
        -0.2353829f, 0.3812674f, 0.8939967f,
        0.0385857f, -0.9154506f, 0.4005763f,
        0.9711365f, 0.1287843f, 0.2007700f, true
    );

    Quatf q2(m2);
    EXPECT_NEAR(1.f, q2.length(), 0.00001f);
    EXPECT_NEAR_QUAT(-Quatf(0.1117323f, -0.6081319f, -0.1725995f, -0.7667469f), q2, 0.00001f);
    EXPECT_EQ(m2, q2.toMatrix33());


    const Matrix33f m3(
        -0.2353829f, 0.3812674f, 0.8939967f,
        -0.9463119f, -0.2995987f, -0.1213857f,
        0.2215598f, -0.8745718f, 0.4313183f, true
    );

    Quatf q3(m3);
    EXPECT_NEAR(1.f, q3.length(), 0.00001f);
    EXPECT_NEAR_QUAT(Quatf(-0.4733753f, 0.3977743f, -0.3551288f, 0.7011241f), q3, 0.00001f);
    EXPECT_EQ(m3, q3.toMatrix33());

    const Matrix33f m4(
        -0.0242432f, 0.0251046f, 0.9993908f,
        0.3092157f, 0.9508508f, -0.0163843f, 
        -0.9506829f, 0.3086301f, -0.0308144f, true 
    );

    Quatf q4(m4);
    EXPECT_NEAR(1.f, q4.length(), 0.00001f);
    EXPECT_NEAR_QUAT(Quatf(0.688439f, 0.1180259f, 0.7081505f, 0.1031722f), q4, 0.00001f);
    EXPECT_EQ(m4, q4.toMatrix33());
}

}
TEST(XQuat, Equality)
{
    Quatf a4(1.f, 2.f, 3.f, 4.f);
    Quatf b4(0.f, 2.f, 3.f, 4.f);
    Quatf c4(1.f, 0.f, 3.f, 4.f);
    Quatf d4(1.f, 2.f, 0.f, 4.f);
    Quatf e4(1.f, 2.f, 3.f, 0.f);

    EXPECT_NE(a4, b4);
    EXPECT_NE(a4, c4);
    EXPECT_NE(a4, d4);
    EXPECT_NE(a4, e4);

    EXPECT_EQ(a4, a4);
    EXPECT_EQ(b4, b4);
}

TEST(XQuat, UnaryMinus)
{
    Quatf a(1.f, 3.f, 2.f, 4.f);
    Quatf b(-1.f, -3.f, -2.f, -4.f);

    EXPECT_EQ(a, -b);
    EXPECT_EQ(-a, b);
}

TEST(XQuat, Invert)
{
    Quatf a(1.f, 3.f, 2.f, 4.f);
    Quatf b(1.f, -3.f, -2.f, -4.f);

    EXPECT_EQ(a, b.inverted());
    EXPECT_EQ(a.inverted(), b);
}

TEST(XQuat, Length)
{
    Quatf a(2.f, -2.f, -8.f, 3.f);
    EXPECT_EQ(9.f, a.length());

    Quatf d(2.f, 3.f, -4.f, -1.f);
    EXPECT_FLOAT_EQ((a * d).length(), a.length() * d.length());
}

TEST(XQuat, LengthSquared)
{
    Quatf a(2.f, -2.f, -8.f, 3.f);
    EXPECT_EQ(81.f, a.lengthSquared());

    Quatf b = a * ~a;
    Quatf c(a.lengthSquared(), 0.f, 0.f, 0.f);
    EXPECT_EQ(b, c);

    Quatf d(2.f, 3.f, -4.f, -1.f);
    EXPECT_EQ((a * d).lengthSquared(), a.lengthSquared() * d.lengthSquared());
}

TEST(XQuat, Dot)
{
    Quatf a(-1.f, 2.f, -3.f, 4.f);
    Quatf b(8.f, 7.f, 6.f, 5.f);

    EXPECT_EQ(8.f, a.dot(b));
}

TEST(XQuat, Log)
{
    Quatf a(0.2f, 0.455f, 0.232f, 0.789f);

    a = a.log();
    EXPECT_NEAR_VEC3(Vec3f(0.635943174f, 0.324261129f, 1.10276735f), a.v, 0.00001f);
}

TEST(XQuat, Exp)
{
    Quatf a(0.2f, 0.455f, 0.232f, 0.789f);

    a = a.exp();
    EXPECT_NEAR_VEC3(Vec3f(0.390908450f, 0.199320346f, 0.677860975f), a.v, 0.00001f);
}

TEST(XQuat, Normalize)
{
    Quatf a(2.f, -2.f, -8.f, 3.f);
    Quatf b = a.normalized();

    EXPECT_NEAR(b.lengthSquared(), 1.0, 1e-5);
}

TEST(XQuat, Lerp)
{
    Quatf a(0.2f, 0.450f, 0.200f, 0.750f);
    Quatf b(0.2f, 0.800f, 0.600f, 0.200f);

    EXPECT_NEAR_VEC3(Quatf(0.200000f, 0.625f, 0.4f, 0.4749999f).v, a.lerp(0.5f, b).v, 0.00001f);
}

TEST(XQuat, Slerp)
{
    Quatf a(0.2f, 0.450f, 0.200f, 0.750f);
    Quatf b(0.2f, 0.800f, 0.600f, 0.200f);

    EXPECT_NEAR_VEC3(Quatf(0.218870267f, 0.683969557f, 0.437740535f, 0.519816875f).v, a.slerp(0.5f, b).v, 0.00001f);
}

TEST(XQuat, Operators)
{
    Quatf a(0.2f, 0.450f, 0.200f, 0.750f);
    Quatf b(0.2f, 0.800f, 0.600f, 0.200f);

    Quatf c = a - b;

    EXPECT_NEAR_QUAT(Quatf(0.4f, 1.25f, 0.8f, 0.95f), (Quatf(0.2f, 0.45f, 0.2f, 0.75f) + Quatf(0.2f, 0.8f, 0.6f, 0.2f)), 0.00001f);
    EXPECT_NEAR_QUAT(Quatf(0.f, -0.35f, -0.4f, 0.55f), (Quatf(0.2f, 0.45f, 0.2f, 0.75f) - Quatf(0.2f, 0.8f, 0.6f, 0.2f)), 0.00001f);
    EXPECT_NEAR_QUAT(Quatf(-0.590000033f, 0.660000026f, -0.350000024f, 0.0800000131f), (Quatf(0.2f, 0.450f, 0.200f, 0.750f) * Quatf(0.2f, 0.800f, 0.600f, 0.200f)), 0.00001f);

    EXPECT_NEAR_QUAT(Quatf(0.4f, 1.25f, 0.8f, 0.95f), (Quatf(0.2f, 0.45f, 0.2f, 0.75f) += Quatf(0.2f, 0.8f, 0.6f, 0.2f)), 0.00001f);
    EXPECT_NEAR_QUAT(Quatf(-0.590000033f, 0.660000026f, -0.350000024f, 0.0800000131f), (Quatf(0.2f, 0.450f, 0.200f, 0.750f) *= Quatf(0.2f, 0.800f, 0.600f, 0.200f)), 0.00001f);
    EXPECT_NEAR_QUAT(Quatf(0.f, -0.35f, -0.4f, 0.55f), (Quatf(0.2f, 0.45f, 0.2f, 0.75f) -= Quatf(0.2f, 0.8f, 0.6f, 0.2f)), 0.00001f);
}