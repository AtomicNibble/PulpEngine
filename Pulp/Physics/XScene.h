#pragma once

#include <IPhysics.h>

X_NAMESPACE_BEGIN(physics)

class PhysXVars;
class DebugRender;

class XScene : public IScene
    , public physx::PxSimulationEventCallback
    , public physx::PxControllerBehaviorCallback
    , public physx::PxUserControllerHitReport
{
    typedef core::Array<TriggerPair, core::ArrayAllocator<TriggerPair>, core::growStrat::Multiply> TriggerPairArr;

public:
    XScene(PhysXVars& vars, physx::PxPhysics* pPhysics, core::MemoryArenaBase* arena);
    ~XScene() X_OVERRIDE;

    bool createPxScene(physx::PxSceneDesc& desc);

    X_INLINE physx::PxScene* getPxScene(void);

    void drawDebug(DebugRender* pDebugRender) const;
    void setVisualizationCullingBox(const AABB& box);

    // locking
    LockHandle lock(LockAccess::Enum access) X_FINAL;
    void unLock(LockHandle lock) X_FINAL;

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

    // set transforms
    void setKinematicTarget(ActorHandle* pHandle, const Transformf* pDestination, size_t num) X_FINAL;
    void setGlobalPose(ActorHandle* pHandle, const Transformf* pDestination, size_t num) X_FINAL;

    // Query stuff
    bool raycast(const Vec3f& origin, const Vec3f& unitDir, const float32_t distance,
        RaycastCallback& hitCall, HitFlags hitFlags, QueryFlags queryFlags) const X_FINAL;

    bool sweep(const GeometryBase& geometry, const Transformf& pose, const Vec3f& unitDir, const float32_t distance,
        SweepCallback& hitCall, HitFlags hitFlags = HitFlag::POSITION | HitFlag::NORMAL | HitFlag::DISTANCE,
        QueryFlags queryFlags = QueryFlag::STATIC | QueryFlag::DYNAMIC,
        const float32_t inflation = 0.f) const X_FINAL;

    bool overlap(const GeometryBase& geometry, const Transformf& pose, OverlapCallback& hitCall) const X_FINAL;

    IBatchedQuery* createBatchQuery(const QueryMemory& desc) X_FINAL;

    // get shit that's moved.
    const ActiveTransform* getActiveTransforms(size_t& numTransformsOut) X_FINAL;
    const TriggerPair* getTriggerPairs(size_t& numTriggerPairs) X_FINAL;

    // PxSimulationEventCallback
    virtual void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) X_FINAL;
    virtual void onWake(physx::PxActor** actors, physx::PxU32 count) X_FINAL;
    virtual void onSleep(physx::PxActor** actors, physx::PxU32 count) X_FINAL;
    virtual void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs) X_FINAL;
    virtual void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) X_FINAL;
    // ~PxSimulationEventCallback

    // PxControllerBehaviorCallback
    virtual physx::PxControllerBehaviorFlags getBehaviorFlags(const physx::PxShape& shape, const physx::PxActor& actor) X_FINAL;
    virtual physx::PxControllerBehaviorFlags getBehaviorFlags(const physx::PxController& controller) X_FINAL;
    virtual physx::PxControllerBehaviorFlags getBehaviorFlags(const physx::PxObstacle& obstacle) X_FINAL;
    // ~PxControllerBehaviorCallback

    // PxUserControllerHitReport
    virtual void onShapeHit(const physx::PxControllerShapeHit& hit) X_FINAL;
    virtual void onControllerHit(const physx::PxControllersHit& hit) X_FINAL;
    virtual void onObstacleHit(const physx::PxControllerObstacleHit& hit) X_FINAL;
    // ~PxUserControllerHitReport

private:
    PhysXVars& vars_;
    core::MemoryArenaBase* arena_;
    physx::PxPhysics* pPhysics_;
    physx::PxScene* pScene_;
    physx::PxControllerManager* pControllerManager_;

    TriggerPairArr triggerPairs_;
};

X_INLINE physx::PxScene* XScene::getPxScene(void)
{
    return pScene_;
}

X_NAMESPACE_END