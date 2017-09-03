#include "EngineCommon.h"
#include "XAabb.h"


AABB::AABB(const OBB& oth)
{
	set(oth);
}


void AABB::set(const OBB &obb)
{
	const Matrix33f o = obb.orientation().toMatrix33();
	const Vec3f& halfVec = obb.halfVec();
	const Vec3f& center = obb.center();

	Vec3f halfSize = Vec3f::abs(o.getColumn(0) * halfVec[0]) +
					 Vec3f::abs(o.getColumn(1) * halfVec[1]) +
					 Vec3f::abs(o.getColumn(2) * halfVec[2]);


	min = center - halfSize;
	max = center + halfSize;
}


AABB AABB::createTransformedAABB(const Quatf& quat, const AABB& aabb)
{
	const Matrix33f o = quat.toMatrix33();
	const Vec3f halfVec = aabb.halfVec();
	const Vec3f center = aabb.center();

	Vec3f halfSize = Vec3f::abs(o.getColumn(0) * halfVec[0]) +
					 Vec3f::abs(o.getColumn(1) * halfVec[1]) +
					 Vec3f::abs(o.getColumn(2) * halfVec[2]);

	return AABB(center - halfSize, center + halfSize);
}


AABB AABB::createTransformedAABB(const Matrix33f& m33, const AABB& aabb)
{
	const Vec3f halfVec = aabb.halfVec();
	const Vec3f center = aabb.center();

	Vec3f halfSize = Vec3f::abs(m33.getColumn(0) * halfVec[0]) +
					 Vec3f::abs(m33.getColumn(1) * halfVec[1]) +
					 Vec3f::abs(m33.getColumn(2) * halfVec[2]);

	return AABB(center - halfSize, center + halfSize);
}
