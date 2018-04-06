#include "stdafx.h"
#include "Math\XWinding.h"

#include "Math\XAabb.h"

X_USING_NAMESPACE;

TEST(Winding, Constructor)
{
    XWinding base;
    AABB aabb, emtpy;

    emtpy.clear();
    base.GetAABB(aabb);

    EXPECT_EQ(0, base.getNumPoints());
    EXPECT_EQ(0, base.getArea());
    EXPECT_EQ(false, base.isHuge());
    EXPECT_EQ(true, base.isTiny());
    EXPECT_EQ(emtpy.min, aabb.min);
    EXPECT_EQ(emtpy.max, aabb.max);
}

TEST(Winding, ConstructorNum)
{
    XWinding base(16);
    AABB aabb, emtpy;

    emtpy.clear();
    base.GetAABB(aabb);

    EXPECT_EQ(0, base.getNumPoints());
    EXPECT_EQ(16, base.getAllocatedSize());
    EXPECT_EQ(0, base.getArea());
    EXPECT_EQ(false, base.isHuge());
    EXPECT_EQ(true, base.isTiny());
    EXPECT_EQ(emtpy.min, aabb.min);
    EXPECT_EQ(emtpy.max, aabb.max);

    base.clear();

    EXPECT_EQ(0, base.getNumPoints());
    EXPECT_EQ(16, base.getAllocatedSize());
    EXPECT_EQ(false, base.isHuge());
    EXPECT_EQ(true, base.isTiny());

    base.free();

    EXPECT_EQ(0, base.getNumPoints());
    EXPECT_EQ(0, base.getAllocatedSize());
    EXPECT_EQ(false, base.isHuge());
    EXPECT_EQ(true, base.isTiny());
}

TEST(Winding, ConstructorVerts)
{
    Vec3f verts[8] = {
        Vec3f(10, 10, 20),
        Vec3f(10, 0, 20),
        Vec3f(0, 10, 20),
        Vec3f(0, 0, 20),
        Vec3f(10, 10, 0),
        Vec3f(10, 0, 0),
        Vec3f(0, 10, 0),
        Vec3f(0, 0, 0)};

    XWinding base(verts, 8);
    AABB aabb, emtpy;

    emtpy.clear();
    base.GetAABB(aabb);

    // 20 units height.
    // 10x10 L + D
    float expected_area = 0.f;

    // top bottom
    // expected_area += (10 * 10) * 4;
    // 4 sides
    // expected_area += (20 * 10) * 2;

    expected_area = 603.225f;

    EXPECT_EQ(8, base.getNumPoints());
    EXPECT_EQ(8, base.getAllocatedSize());
    EXPECT_FLOAT_EQ(expected_area, base.getArea());
    EXPECT_EQ(false, base.isHuge());
    EXPECT_EQ(false, base.isTiny());
    EXPECT_EQ(Vec3f(0, 0, 0), aabb.min);
    EXPECT_EQ(Vec3f(10, 10, 20), aabb.max);
    EXPECT_EQ(Vec3f(5, 5, 10), base.getCenter());

    base.clear();

    EXPECT_EQ(0, base.getNumPoints());
    EXPECT_EQ(8, base.getAllocatedSize());
    EXPECT_EQ(false, base.isHuge());
    EXPECT_EQ(true, base.isTiny());

    base.free();

    EXPECT_EQ(0, base.getNumPoints());
    EXPECT_EQ(0, base.getAllocatedSize());
    EXPECT_EQ(false, base.isHuge());
    EXPECT_EQ(true, base.isTiny());
}

TEST(Winding, ConstructorPlane)
{
    // hoz plane going through 0,0,0
    Planef plane_out, plane(Vec3f(0, 0, 0), Vec3f(10, 0, 0), Vec3f(10, 10, 0));
    XWinding base(plane);
    AABB aabb;

    // we should have a huge ass plane, size of max world.
    // represented by 4 points.
    base.GetAABB(aabb);
    base.getPlane(plane_out);

    // TODO expose this in some header.
    // it's currently defined in 2-3 places.
    static const float MAX_WORLD = 262144;
    const float area = (MAX_WORLD * 2) * (MAX_WORLD * 2);

    ASSERT_EQ(4, base.getNumPoints());
    EXPECT_EQ(4, base.getAllocatedSize());
    EXPECT_FLOAT_EQ(area, base.getArea());
    EXPECT_EQ(true, base.isHuge());
    EXPECT_EQ(false, base.isTiny());
    EXPECT_EQ(Vec3f(0, 0, 0), base.getCenter());
    EXPECT_EQ(Vec3f(-MAX_WORLD, -MAX_WORLD, 0), aabb.min);
    EXPECT_EQ(Vec3f(MAX_WORLD, MAX_WORLD, 0), aabb.max);

    Vec3f points[4] = {
        Vec3f(MAX_WORLD, -MAX_WORLD, 0),
        Vec3f(MAX_WORLD, MAX_WORLD, 0),
        Vec3f(-MAX_WORLD, MAX_WORLD, 0),
        Vec3f(-MAX_WORLD, -MAX_WORLD, 0)};

    for (int i = 0; i < 4; i++) {
        EXPECT_EQ(points[i], base[i].asVec3());
    }
}

TEST(Winding, ConstructorPlane2)
{
    // hoz plane going through 0,0,0
    Planef plane_out, plane(Vec3f(0, 0, 0), Vec3f(10, 0, 0), Vec3f(10, 10, 0));
    XWinding base(plane.getNormal(), plane.getDistance());
    AABB aabb;

    // we should have a huge ass plane, size of max world.
    // represented by 4 points.
    base.GetAABB(aabb);
    base.getPlane(plane_out);

    // TODO expose this in some header.
    // it's currently defined in 2-3 places.
    static const float MAX_WORLD = 262144;
    const float area = (MAX_WORLD * 2) * (MAX_WORLD * 2);

    ASSERT_EQ(4, base.getNumPoints());
    EXPECT_EQ(4, base.getAllocatedSize());
    EXPECT_FLOAT_EQ(area, base.getArea());
    EXPECT_EQ(true, base.isHuge());
    EXPECT_EQ(false, base.isTiny());
    EXPECT_EQ(Vec3f(0, 0, 0), base.getCenter());
    EXPECT_EQ(Vec3f(-MAX_WORLD, -MAX_WORLD, 0), aabb.min);
    EXPECT_EQ(Vec3f(MAX_WORLD, MAX_WORLD, 0), aabb.max);

    Vec3f points[4] = {
        Vec3f(MAX_WORLD, -MAX_WORLD, 0),
        Vec3f(MAX_WORLD, MAX_WORLD, 0),
        Vec3f(-MAX_WORLD, MAX_WORLD, 0),
        Vec3f(-MAX_WORLD, -MAX_WORLD, 0)};

    for (int i = 0; i < 4; i++) {
        EXPECT_EQ(points[i], base[i].asVec3());
    }
}

TEST(Winding, ConstructorSelf)
{
    Planef plane_out, plane(Vec3f(0, 0, 0), Vec3f(10, 0, 0), Vec3f(10, 10, 0));
    XWinding temp(plane);

    // create the winding from temp.
    XWinding base(temp);

    AABB aabb;

    // we should have a huge ass plane, size of max world.
    // represented by 4 points.
    base.GetAABB(aabb);
    base.getPlane(plane_out);

    // TODO expose this in some header.
    // it's currently defined in 2-3 places.
    static const float MAX_WORLD = 262144;
    const float area = (MAX_WORLD * 2) * (MAX_WORLD * 2);

    ASSERT_EQ(4, base.getNumPoints());
    EXPECT_EQ(4, base.getAllocatedSize());
    EXPECT_FLOAT_EQ(area, base.getArea());
    EXPECT_EQ(true, base.isHuge());
    EXPECT_EQ(false, base.isTiny());
    EXPECT_EQ(Vec3f(0, 0, 0), base.getCenter());
    EXPECT_EQ(Vec3f(-MAX_WORLD, -MAX_WORLD, 0), aabb.min);
    EXPECT_EQ(Vec3f(MAX_WORLD, MAX_WORLD, 0), aabb.max);

    Vec3f points[4] = {
        Vec3f(MAX_WORLD, -MAX_WORLD, 0),
        Vec3f(MAX_WORLD, MAX_WORLD, 0),
        Vec3f(-MAX_WORLD, MAX_WORLD, 0),
        Vec3f(-MAX_WORLD, -MAX_WORLD, 0)};

    for (int i = 0; i < 4; i++) {
        EXPECT_EQ(points[i], base[i].asVec3());
    }
}

TEST(Winding, PlaneDistance)
{
    // trusty box of goats.
    Vec3f verts[8] = {
        Vec3f(10, 10, 10),
        Vec3f(10, 0, 10),
        Vec3f(0, 10, 10),
        Vec3f(0, 0, 10),
        Vec3f(10, 10, -10),
        Vec3f(10, 0, -10),
        Vec3f(0, 10, -10),
        Vec3f(0, 0, -10)};

    XWinding winding(verts, 8);

    // show me the distance!
    struct PlaneMap
    {
        Planef plane;
        float expectedDistance;
    };

    static const int numPlanes = 4;
    PlaneMap info[numPlanes] = {
        {Planef(Vec3f(100, 0, 0), Vec3f(100, 10, 0), Vec3f(100, 0, -10)), -90.f}, // 90?
        // gonna be 40 x ¹2 which is: 56.5685424949238
        {Planef(Vec3f(50, 50, 0), Vec3f(60, 40, 0), Vec3f(40, 60, 10)), -56.5685424949f},

        {Planef(Vec3f(5, 5, 0), Vec3f(0, 0, 0), Vec3f(0, 0, 15)), 0}, // zero also
        {Planef(Vec3f(0, 0, 0), Vec3f(0, 0, 0), Vec3f(0, 0, 0)), 0}   // zero

    };

    int i;
    for (i = 0; i < numPlanes; i++) {
        EXPECT_FLOAT_EQ(info[i].expectedDistance, winding.planeDistance(info[i].plane));
    }
}

TEST(Winding, PlaneSide)
{
    // trusty box of goats.
    Vec3f verts[8] = {
        Vec3f(10, 10, 10),
        Vec3f(10, 0, 10),
        Vec3f(0, 10, 10),
        Vec3f(0, 0, 10),
        Vec3f(10, 10, -10),
        Vec3f(10, 0, -10),
        Vec3f(0, 10, -10),
        Vec3f(0, 0, -10)};

    XWinding winding(verts, 8);

    // show me the distance!
    struct PlaneMap
    {
        Planef plane;
        PlaneSide::Enum expectedSide;
    };

    static const int numPlanes = 4;
    PlaneMap info[numPlanes] = {
        {Planef(Vec3f(100, 0, 0), Vec3f(100, 10, 0), Vec3f(100, 0, -10)), PlaneSide::BACK},
        {Planef(Vec3f(50, 50, 0), Vec3f(60, 40, 0), Vec3f(40, 60, 10)), PlaneSide::BACK},
        {Planef(Vec3f(5, 5, 0), Vec3f(0, 0, 0), Vec3f(0, 0, 15)), PlaneSide::CROSS},
        {Planef(Vec3f(0, 0, 0), Vec3f(0, 0, 0), Vec3f(0, 0, 0)), PlaneSide::ON}};

    int i;
    for (i = 0; i < numPlanes; i++) {
        EXPECT_EQ(info[i].expectedSide, winding.planeSide(info[i].plane));
    }
}
