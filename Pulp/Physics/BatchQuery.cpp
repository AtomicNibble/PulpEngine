#include "stdafx.h"
#include "BatchQuery.h"
#include "Util/MathHelpers.h"

#include <PxBatchQuery.h>

X_NAMESPACE_BEGIN(physics)

BatchedQuery::BatchedQuery(physx::PxBatchQuery* pBatchedQuery, core::MemoryArenaBase* arena) :
    pBatchedQuery_(pBatchedQuery),
    arena_(arena)
{
}

BatchedQuery::~BatchedQuery()
{
    core::SafeRelease(pBatchedQuery_);
}

void BatchedQuery::execute(void)
{
    pBatchedQuery_->execute();
}

void BatchedQuery::release(void)
{
    // we need to delete ourself :O
    X_DELETE(this, arena_);
}

void BatchedQuery::raycast(const Vec3f& origin, const Vec3f& unitDir, const float32_t distance,
    int16_t maxtouchHits, HitFlags hitFlags, QueryFlags queryFlags) const
{
    pBatchedQuery_->raycast(
        Px3FromVec3(origin),
        Px3FromVec3(unitDir),
        distance,
        maxtouchHits,
        static_cast<physx::PxHitFlags>(hitFlags.ToInt()),
        physx::PxQueryFilterData(static_cast<physx::PxQueryFlags>(queryFlags.ToInt())));
}

void BatchedQuery::sweep(const GeometryBase& geometry, const Transformf& pose, const Vec3f& unitDir, const float32_t distance,
    int16_t maxTouchHits, HitFlags hitFlags, QueryFlags queryFlags, const float32_t inflation) const
{
    return pBatchedQuery_->sweep(
        PxGeoFromGeo(geometry),
        PxTransFromQuatTrans(pose),
        Px3FromVec3(unitDir),
        distance,
        maxTouchHits,
        static_cast<physx::PxHitFlags>(hitFlags.ToInt()),
        physx::PxQueryFilterData(static_cast<physx::PxQueryFlags>(queryFlags.ToInt())));
}

void BatchedQuery::overlap(const GeometryBase& geometry, const Transformf& pose, int16_t maxTouchHits) const
{
    return pBatchedQuery_->overlap(
        PxGeoFromGeo(geometry),
        PxTransFromQuatTrans(pose),
        maxTouchHits);
}

X_NAMESPACE_END