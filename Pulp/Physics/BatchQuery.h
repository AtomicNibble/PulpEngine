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
	BatchedQuery(physx::PxBatchQuery* pBatchedQuery);
	~BatchedQuery();

	void execute(void) X_FINAL;
	void release(void) X_FINAL;

	void raycast(const Vec3f& origin, const Vec3f& unitDir, const float32_t distance,
		int16_t maxtouchHits, HitFlags hitFlags = HitFlag::POSITION | HitFlag::NORMAL | HitFlag::DISTANCE) const X_FINAL;

	void sweep(const GeometryBase& geometry, const QuatTransf& pose, const Vec3f& unitDir, const float32_t distance,
		int16_t maxTouchHits, HitFlags hitFlags = HitFlag::POSITION | HitFlag::NORMAL | HitFlag::DISTANCE,
		const float32_t inflation = 0.f) const X_FINAL;

	void overlap(const GeometryBase& geometry, const QuatTransf& pose, int16_t maxTouchHits) const X_FINAL;



private:
	physx::PxBatchQuery* pBatchedQuery_;
};

X_NAMESPACE_END