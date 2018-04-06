#include "stdafx.h"

X_USING_NAMESPACE;

#if 0

// duplicate the size check.
X_ENSURE_SIZE(Vec3fA, 16);
X_ENSURE_SIZE(Vec4fA, 16);


TEST(XVectorAligned, Constructor)
{
	float vals[] = { 546.f, 41.f, 442.f, 576.f };

	Vec4fA a(4.f, 44.f, 3.f, 0.f);
	Vec4fA b;

	X_ASSERT_ALIGNMENT(&a, 16, 0);
	X_ASSERT_ALIGNMENT(&b, 16, 0);

	EXPECT_FLOAT_EQ(4.f, a[0]);
	EXPECT_FLOAT_EQ(44.f, a[1]);
	EXPECT_FLOAT_EQ(3.f, a[2]);
	EXPECT_FLOAT_EQ(0.f, a[3]);

	EXPECT_FLOAT_EQ(0.f, b[0]);
	EXPECT_FLOAT_EQ(0.f, b[1]);
	EXPECT_FLOAT_EQ(0.f, b[2]);
	EXPECT_FLOAT_EQ(0.f, b[3]);
}

TEST(XVectorAligned, Invert)
{
	Vec4fA a(546.f, 41.f, 442.f, -576.f);
	Vec4fA b = a.inverse();

	a.invert();

	EXPECT_FLOAT_EQ(-546.f, a[0]);
	EXPECT_FLOAT_EQ(-41.f, a[1]);
	EXPECT_FLOAT_EQ(-442.f, a[2]);
	EXPECT_FLOAT_EQ(576.f, a[3]);

	EXPECT_FLOAT_EQ(-546.f, b[0]);
	EXPECT_FLOAT_EQ(-41.f, b[1]);
	EXPECT_FLOAT_EQ(-442.f, b[2]);
	EXPECT_FLOAT_EQ(576.f, b[3]);
}


TEST(XVectorAligned, Normal)
{
	Vec4fA a(546.f, 41.f, 442.f, 0.f);
	
	a.normalize();
	float length = a.length();
	float lengthSq = a.lengthSquared();


	EXPECT_FLOAT_EQ(0.775924385f, a[0]);
	EXPECT_FLOAT_EQ(0.0582653880f, a[1]);
	EXPECT_FLOAT_EQ(0.628129303f, a[2]);
	EXPECT_FLOAT_EQ(0.f, a[3]);

	EXPECT_FLOAT_EQ(1.f, length);
	EXPECT_FLOAT_EQ(1.f, lengthSq);
}


TEST(XVectorAligned, Length)
{
	Vec4fA a(546.f, 41.f, 442.f, 0.f);

	float length = a.length();
	float lengthSq = a.lengthSquared();

	EXPECT_FLOAT_EQ(703.676758f, length);
	EXPECT_FLOAT_EQ(495161.f, lengthSq);
}


TEST(XVectorAligned, Distance)
{
	Vec4fA a(546.f, 41.f, 442.f, 0.f);
	Vec4fA b(4.f, 44.f, 3.f, 0.f);

	float distance = a.distance(b);
	float distanceSq = a.distanceSquared(b);

	EXPECT_FLOAT_EQ(697.491211f, distance);
	EXPECT_FLOAT_EQ(486494.f, distanceSq);
}


TEST(XVectorAligned, Cross)
{
	Vec4fA a3(4, 8, 10);
	Vec4fA b3(2, 2, 1);

	EXPECT_TRUE(Vec4fA(-12, 16, -8).compare(a3.cross(b3)));

}

TEST(XVectorAligned, Arithmic) 
{

	// test some basic booty shaking.

	// addition.
	EXPECT_TRUE(Vec3fA(5, 5, 10) == ( Vec3fA(1, 4, 1) + Vec3fA(4, 1, 9)));
	EXPECT_TRUE(Vec4fA(5, 5, 10, 15) == (Vec4fA(2, 3, 5, 7) + Vec4fA(3, 2, 5, 8)));

	// subtraction.
	EXPECT_TRUE(Vec3fA(-3, 3, -8) == (Vec3fA(1, 4, 1) - Vec3fA(4, 1, 9)));
	EXPECT_TRUE(Vec4fA(-1, 1, 0, -1) == (Vec4fA(2, 3, 5, 7) - Vec4fA(3, 2, 5, 8)));

	// division.
	EXPECT_TRUE(Vec3fA(0.25f, 4, 0.1111111f) == (Vec3fA(1, 4, 1) / Vec3fA(4, 1, 9)));
	EXPECT_TRUE(Vec4fA(0.666666f, 1.5f, 1, 0.875f) == (Vec4fA(2, 3, 5, 7) / Vec4fA(3, 2, 5, 8)));

	// need to test assign operators.
	// we just need to add the '=' it will make the correct code be used.

	// addition.
	EXPECT_TRUE(Vec3fA(5, 5, 10) == (Vec3fA(1, 4, 1) += Vec3fA(4, 1, 9)));
	EXPECT_TRUE(Vec4fA(5, 5, 10, 15) == (Vec4fA(2, 3, 5, 7) += Vec4fA(3, 2, 5, 8)));

	// subtraction.
	EXPECT_TRUE(Vec3fA(-3, 3, -8) == (Vec3fA(1, 4, 1) -= Vec3fA(4, 1, 9)));
	EXPECT_TRUE(Vec4fA(-1, 1, 0, -1) == (Vec4fA(2, 3, 5, 7) -= Vec4fA(3, 2, 5, 8)));

	// multiplication.
	EXPECT_TRUE(Vec3fA(4, 4, 9) == (Vec3fA(1, 4, 1) *= Vec3fA(4, 1, 9)));
	EXPECT_TRUE(Vec4fA(6, 6, 25, 56) == (Vec4fA(2, 3, 5, 7) *= Vec4fA(3, 2, 5, 8)));

	// division.
	EXPECT_TRUE(Vec3fA(0.25f, 4, 0.1111111f) == (Vec3fA(1, 4, 1) /= Vec3fA(4, 1, 9)));
	EXPECT_TRUE(Vec4fA(0.666666f, 1.5f, 1, 0.875f) == (Vec4fA(2, 3, 5, 7) /= Vec4fA(3, 2, 5, 8)));

}

TEST(XVectorAligned, Dot)
{

	Vec3fA a3(4, 8, 10);
	Vec3fA b3(2, 2, 1);

	EXPECT_EQ(34, a3.dot(b3));

	Vec4fA a4(4, 8, 25, 4);
	Vec4fA b4(2, 2, 14, 4);

	EXPECT_EQ(390, a4.dot(b4));
}


TEST(XVectorAligned, UnaryMinus) 
{

	Vec3fA a3(1.f, 3.f, 5.f);
	Vec3fA b3(-1.f, -3.f, -5.f);

	EXPECT_TRUE(a3 == -b3);
	EXPECT_TRUE(-a3 == b3);

	Vec4fA a4(1.f, 3.f, 5.f, 8.f);
	Vec4fA b4(-1.f, -3.f, -5.f, -8.f);

	EXPECT_TRUE(a4 == -b4);
	EXPECT_TRUE(-a4 == b4);
}

TEST(XVectorAligned, Equality)
{
	Vec3fA a3(1.f, 2.f, 3.f);
	Vec3fA b3(0.f, 2.f, 3.f);
	Vec3fA c3(1.f, 0.f, 3.f);
	Vec3fA d3(1.f, 2.f, 0.f);

	EXPECT_FALSE(a3 == b3);
	EXPECT_FALSE(a3 == c3);
	EXPECT_FALSE(a3 == d3);

	EXPECT_TRUE(a3 == a3);
	EXPECT_TRUE(b3 == b3);

	Vec4fA a4(1.f, 2.f, 3.f, 4.f);
	Vec4fA b4(0.f, 2.f, 3.f, 4.f);
	Vec4fA c4(1.f, 0.f, 3.f, 4.f);
	Vec4fA d4(1.f, 2.f, 0.f, 4.f);
	Vec4fA e4(1.f, 2.f, 3.f, 0.f);

	EXPECT_FALSE(a4 == b4);
	EXPECT_FALSE(a4 == c4);
	EXPECT_FALSE(a4 == d4);
	EXPECT_FALSE(a4 == e4);

	EXPECT_TRUE(a4 == a4);
	EXPECT_TRUE(d4 == d4);
}

#endif