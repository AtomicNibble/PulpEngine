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