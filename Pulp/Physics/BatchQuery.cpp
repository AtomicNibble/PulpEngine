#include "stdafx.h"
#include "BatchQuery.h"
#include "MathHelpers.h"

#include <PxBatchQuery.h>


X_NAMESPACE_BEGIN(physics)

BatchedQuery::BatchedQuery(physx::PxBatchQuery* pBatchedQuery) :
	pBatchedQuery_(pBatchedQuery)
{

}

BatchedQuery::~BatchedQuery()
{
	release();
}

void BatchedQuery::execute(void)
{
	pBatchedQuery_->execute();
}

void BatchedQuery::release(void)
{
	// Meoooow!
	// we need to delete ourself :O

	core::SafeRelease(pBatchedQuery_);
}

void BatchedQuery::raycast(const Vec3f& origin, const Vec3f& unitDir, const float32_t distance,
	int16_t maxtouchHits, HitFlags hitFlags) const
{
	pBatchedQuery_->raycast(
		Px3FromVec3(origin),
		Px3FromVec3(unitDir),
		distance,
		maxtouchHits,
		static_cast<physx::PxHitFlags>(hitFlags.ToInt())
	);

}

void BatchedQuery::sweep(const GeometryBase& geometry, const QuatTransf& pose, const Vec3f& unitDir, const float32_t distance,
	int16_t maxTouchHits, HitFlags hitFlags, const float32_t inflation) const
{
	return pBatchedQuery_->sweep(
		PxGeoFromGeo(geometry),
		PxTransFromQuatTrans(pose),
		Px3FromVec3(unitDir),
		distance,
		maxTouchHits,
		static_cast<physx::PxHitFlags>(hitFlags.ToInt())
	);
}

void BatchedQuery::overlap(const GeometryBase& geometry, const QuatTransf& pose, int16_t maxTouchHits) const
{
	return pBatchedQuery_->overlap(
		PxGeoFromGeo(geometry),
		PxTransFromQuatTrans(pose),
		maxTouchHits
	);

}





X_NAMESPACE_END