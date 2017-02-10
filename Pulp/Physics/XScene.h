#pragma once

#include <IPhysics.h>


X_NAMESPACE_BEGIN(physics)

class PhysXVars;
class DebugRender;

class XScene : public IScene
{
public:
	XScene(PhysXVars& vars, physx::PxPhysics* pPhysics, core::MemoryArenaBase* arena);
	~XScene() X_OVERRIDE;

	bool createPxScene(const physx::PxSceneDesc& desc);

	X_INLINE physx::PxScene* getPxScene(void);

	void drawDebug(DebugRender* pDebugRender) const;
	void setVisualizationCullingBox(const AABB& box);

	// some runtime tweaks.
	void setGravity(const Vec3f& gravity) X_FINAL;
	void setBounceThresholdVelocity(float32_t bounceThresholdVelocity) X_FINAL;
	// ~

	// region's
	RegionHandle addRegion(const AABB& bounds) X_FINAL;
	bool removeRegion(RegionHandle handles) X_FINAL;

	void addActorToScene(ActorHandle handle) X_FINAL;
	void addActorToScene(ActorHandle handle, const char* pDebugNamePointer) X_FINAL;
	void addActorsToScene(ActorHandle* pHandles, size_t num) X_FINAL;
	void removeActor(ActorHandle handle) X_FINAL;
	void removeActors(ActorHandle* pHandles, size_t num) X_FINAL;

	// Aggregate
	void addAggregate(AggregateHandle handle) X_FINAL;
	void removeAggregate(AggregateHandle handle) X_FINAL;

	// Characters controllers
	ICharacterController* createCharacterController(const ControllerDesc& desc) X_FINAL;
	void releaseCharacterController(ICharacterController* pController) X_FINAL;


	// Query stuff
	bool raycast(const Vec3f& origin, const Vec3f& unitDir, const float32_t distance,
		RaycastCallback& hitCall, HitFlags hitFlags) const X_FINAL;

	bool sweep(const GeometryBase& geometry, const Transformf& pose, const Vec3f& unitDir, const float32_t distance,
		SweepCallback& hitCall, HitFlags hitFlags = HitFlag::POSITION | HitFlag::NORMAL | HitFlag::DISTANCE,
		const float32_t inflation = 0.f) const X_FINAL;

	bool overlap(const GeometryBase& geometry, const Transformf& pose, OverlapCallback& hitCall) const X_FINAL;

	IBatchedQuery* createBatchQuery(const QueryMemory& desc) X_FINAL;

	// get shit that's moved.
	const ActiveTransform* getActiveTransforms(size_t& numTransformsOut) X_FINAL;


private:
	PhysXVars& vars_;
	core::MemoryArenaBase* arena_;
	physx::PxPhysics* pPhysics_;
	physx::PxScene*	pScene_;
	physx::PxControllerManager*	pControllerManager_;
};

X_INLINE physx::PxScene* XScene::getPxScene(void)
{
	return pScene_;
}

X_NAMESPACE_END