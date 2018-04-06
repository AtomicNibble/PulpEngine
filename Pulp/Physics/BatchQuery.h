#pragma once

#include <IPhysics.h>

namespace physx
{
    class PxBatchQuery;
}

X_NAMESPACE_BEGIN(physics)

class BatchedQuery : public IBatchedQuery
{
public:
    BatchedQuery(physx::PxBatchQuery* pBatchedQuery, core::MemoryArenaBase* arena);
    ~BatchedQuery();

    void execute(void) X_FINAL;
    void release(void) X_FINAL;

    void raycast(const Vec3f& origin, const Vec3f& unitDir, const float32_t distance,
        int16_t maxtouchHits, HitFlags hitFlags = HitFlag::POSITION | HitFlag::NORMAL | HitFlag::DISTANCE,
        QueryFlags queryFlags = QueryFlag::STATIC | QueryFlag::DYNAMIC) const X_FINAL;

    void sweep(const GeometryBase& geometry, const Transformf& pose, const Vec3f& unitDir, const float32_t distance,
        int16_t maxTouchHits, HitFlags hitFlags = HitFlag::POSITION | HitFlag::NORMAL | HitFlag::DISTANCE,
        QueryFlags queryFlags = QueryFlag::STATIC | QueryFlag::DYNAMIC,
        const float32_t inflation = 0.f) const X_FINAL;

    void overlap(const GeometryBase& geometry, const Transformf& pose, int16_t maxTouchHits) const X_FINAL;

private:
    physx::PxBatchQuery* pBatchedQuery_;
    core::MemoryArenaBase* arena_;
};

X_NAMESPACE_END