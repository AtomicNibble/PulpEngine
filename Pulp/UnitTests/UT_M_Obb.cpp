#include "stdafx.h"
#include "Math\XObb.h"
#include <gtest\gtest.h>



TEST(Obb, Set)
{
	Vec3f center(50.f);
	Vec3f half(5.f);
	OBB  obb;

	// create a rotation.
	Matrix33f mat = Matrix33f::identity();
	mat.rotate(Vec3f(50.f, 79.f, 12.f));

	obb.set(mat, center, half);

	EXPECT_EQ(center, obb.center());
	EXPECT_EQ(half, obb.halfVec());

	// matrix code is unit tested, just check that it's been assigned.
	EXPECT_EQ(mat, obb.orientation());
}

TEST(Obb, SetMat)
{
	AABB aabb(Vec3f(5.f), Vec3f(10.f)); // good old 5-10
	OBB  obb;

	// create a rotation.
	Matrix33f mat = Matrix33f::identity();
	mat.rotate(Vec3f(50.f, 79.f, 12.f));

	obb.set(mat, aabb);


	EXPECT_EQ(Vec3f(7.5f), obb.center());
	EXPECT_EQ(Vec3f(2.5f), obb.halfVec());

	// matrix code is unit tested, just check that it's been assigned.
	EXPECT_EQ(mat, obb.orientation());
}

TEST(Obb, SetQuat)
{
	AABB aabb(Vec3f(5.f), Vec3f(15.f, 105.f, 865.f)); // something diffrent heeh
	OBB  obb;

	// create a rotation.
	Matrix33f mat = Matrix33f::identity();
	mat.rotate(Vec3f(50.f, 79.f, 12.f));

	obb.set(Quatf(mat), aabb);

	EXPECT_EQ(Vec3f(10.f, 55.f, 435.f), obb.center());
	EXPECT_EQ(Vec3f(5.f, 50.f, 430.f), obb.halfVec());

	// matrix code is unit tested, just check that it's been assigned.
	EXPECT_EQ(mat, obb.orientation());
}