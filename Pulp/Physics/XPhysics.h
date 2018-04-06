#pragma once

#include <IPhysics.h>

#include "extensions/PxExtensionsAPI.h"
#include "physxvisualdebuggersdk/PvdConnectionManager.h"

#include "Util/Allocator.h"
#include "Util/Logger.h"
#include "Util/CpuDispatcher.h"
#include "Vars/PhysicsVars.h"
#include "DebugRender/DebugRender.h"
#include "Stepper.h"

namespace PVD
{
    using namespace physx::debugger;
    using namespace physx::debugger::comm;
} // namespace PVD

X_NAMESPACE_DECLARE(core,
                    struct IConsoleCmdArgs;)

X_NAMESPACE_BEGIN(physics)

class PhysCooking;
class XScene;

class XPhysics : public IPhysics
    ,
                 public PVD::PvdConnectionHandler
    , //receive notifications when pvd is connected and disconnected.
                 public physx::PxDeletionListener
    , public physx::PxBroadPhaseCallback
    , public IStepperHandler
{
    X_NO_COPY(XPhysics);
    X_NO_ASSIGN(XPhysics);

public:
    static const size_t SCRATCH_BLOCK_SIZE = 1024 * 16;
    static const physx::PxShapeFlags DEFALT_SHAPE_FLAGS;

    typedef core::FixedArray<XScene*, MAX_ACTIVE_SCENES> ActiveSceneListArr;
    typedef core::FixedArray<XScene*, MAX_SCENES> SceneListArr;

    typedef core::Array<physx::PxActor*> ActorsArr;

public:
    XPhysics(uint32_t maxSubSteps, core::V2::JobSystem* pJobSys, core::MemoryArenaBase* arena);
    ~XPhysics() X_OVERRIDE;

    // IPhysics
    void registerVars(void) X_FINAL;
    void registerCmds(void) X_FINAL;

    bool init(const ToleranceScale& scale) X_FINAL;
    void shutDown(void) X_FINAL;
    void release(void) X_FINAL;

    void onTickPreRender(core::TimeVal dtime, const AABB& debugVisCullBounds) X_FINAL;
    void onTickPostRender(core::TimeVal dtime) X_FINAL;
    void render(void) X_FINAL;

    IPhysicsCooking* getCooking(void) X_FINAL;

    // Scene stuff
    IScene* createScene(const SceneDesc& desc) X_FINAL;
    void addSceneToSim(IScene* pScene) X_FINAL;
    bool removeSceneFromSim(IScene* pScene) X_FINAL;
    void releaseScene(IScene* pScene) X_FINAL;

    // materials
    MaterialHandle createMaterial(MaterialDesc& desc) X_FINAL;
    MaterialHandle getDefaultMaterial(void) X_FINAL;

    // aggregates's
    AggregateHandle createAggregate(uint32_t maxActors, bool selfCollisions) X_FINAL;
    bool addActorToAggregate(AggregateHandle handle, ActorHandle actor) X_FINAL;
    bool releaseAggregate(AggregateHandle handle) X_FINAL;

    // joints
    IJoint* createJoint(JointType::Enum type, ActorHandle actor0, ActorHandle actor1,
        const Transformf& localFrame0, const Transformf& localFrame1) X_FINAL;
    void releaseJoint(IJoint* pJoint) X_FINAL;

    void setActorDebugNamePointer(ActorHandle handle, const char* pNamePointer) X_FINAL;
    void setActorDominanceGroup(ActorHandle handle, int8_t group) X_FINAL;
    void setGroup(ActorHandle handle, const GroupFlag::Enum group) X_FINAL;
    void setGroupFlags(ActorHandle handle, const GroupFlags groupFlags) X_FINAL;

    // group collision
    bool GetGroupCollisionFlag(const GroupFlag::Enum group1, const GroupFlag::Enum group2) X_FINAL;
    void SetGroupCollisionFlag(const GroupFlag::Enum group1, const GroupFlag::Enum group2, const bool enable) X_FINAL;

    // you must pass cooked data :|
    // if you don't have cooked data use getCooking() to cook it!
    TriMeshHandle createTriangleMesh(const DataArr& cooked) X_FINAL;
    ConvexMeshHandle createConvexMesh(const DataArr& cooked) X_FINAL;
    ConvexMeshHandle createConvexMesh(const uint8_t* pData, size_t length) X_FINAL;
    HieghtFieldHandle createHieghtField(const DataArr& cooked) X_FINAL;

    ActorHandle createConvexMesh(const Transformf& myTrans, ConvexMeshHandle mesh, float density, const Vec3f& scale = Vec3f::one()) X_FINAL;
    ActorHandle createTriangleMesh(const Transformf& myTrans, TriMeshHandle convex, float density, const Vec3f& scale = Vec3f::one()) X_FINAL;
    ActorHandle createHieghtField(const Transformf& myTrans, HieghtFieldHandle hf, float density, const Vec3f& heightRowColScale = Vec3f::one()) X_FINAL;
    ActorHandle createSphere(const Transformf& myTrans, float radius, float density) X_FINAL;
    ActorHandle createCapsule(const Transformf& myTrans, float radius, float halfHeight, float density) X_FINAL;
    ActorHandle createBox(const Transformf& myTrans, const AABB& bounds, float density) X_FINAL;

    ActorHandle createStaticTriangleMesh(const Transformf& myTrans, TriMeshHandle mesh, const Vec3f& scale = Vec3f::one()) X_FINAL;
    ActorHandle createStaticHieghtField(const Transformf& myTrans, HieghtFieldHandle hf, const Vec3f& heightRowColScale = Vec3f::one()) X_FINAL;
    ActorHandle createStaticPlane(const Transformf& myTrans) X_FINAL;
    ActorHandle createStaticSphere(const Transformf& myTrans, float radius) X_FINAL;
    ActorHandle createStaticCapsule(const Transformf& myTrans, float radius, float halfHeight) X_FINAL;
    ActorHandle createStaticBox(const Transformf& myTrans, const AABB& bounds) X_FINAL;
    ActorHandle createStaticTrigger(const Transformf& myTrans, const AABB& bounds) X_FINAL;

    // for creating a actor without any initial shape.
    ActorHandle createActor(const Transformf& myTrans, bool kinematic, const void* pUserData) X_FINAL;
    ActorHandle createStaticActor(const Transformf& myTrans, const void* pUserData) X_FINAL;

    // call this after all the shapes have been added.
    void updateMassAndInertia(ActorHandle a, float density) X_FINAL;

    // adding additional shapes to a actor.
    void addTriMesh(ActorHandle handle, TriMeshHandle mesh, const Vec3f& scale = Vec3f::one()) X_FINAL;
    void addConvexMesh(ActorHandle handle, ConvexMeshHandle con, const Vec3f& scale) X_FINAL;
    void addHieghtField(ActorHandle handle, HieghtFieldHandle hf, const Vec3f& heightRowColScale = Vec3f::one()) X_FINAL;
    void addBox(ActorHandle handle, const AABB& aabb) X_FINAL;
    void addBox(ActorHandle handle, const AABB& aabb, const Vec3f& localPose) X_FINAL;
    void addSphere(ActorHandle handle, float radius) X_FINAL;
    void addSphere(ActorHandle handle, float radius, const Vec3f& localPose) X_FINAL;
    void addCapsule(ActorHandle handle, float radius, float halfHeight) X_FINAL;
    // ~IPhysics

private:
    // PvdConnectionHandler
    virtual void onPvdSendClassDescriptions(PVD::PvdConnection&) X_FINAL;
    virtual void onPvdConnected(PVD::PvdConnection& inFactory) X_FINAL;
    virtual void onPvdDisconnected(PVD::PvdConnection& inFactory) X_FINAL;
    // ~PvdConnectionHandler

    // ~PxDeletionListener
    virtual void onRelease(const physx::PxBase* observed, void* userData, physx::PxDeletionEventFlag::Enum deletionEvent) X_FINAL;
    // ~PxDeletionListener

    // PxBroadPhaseCallback
    virtual void onObjectOutOfBounds(physx::PxShape& shape, physx::PxActor& actor) X_FINAL;
    virtual void onObjectOutOfBounds(physx::PxAggregate& aggregate) X_FINAL;
    // ~PxBroadPhaseCallback

    // IStepperHandler
    virtual void onSubstepPreFetchResult(void) X_FINAL;
    virtual void onSubstep(float32_t dtTime) X_FINAL;
    virtual void onSubstepSetup(float dtime, physx::PxBaseTask* cont) X_FINAL;
    // ~IStepperHandler

    void PvdSetup(void);
    void PvdCleanup(void);
    void togglePvdConnection(void);
    void closePvdConnection(void);
    void createPvdConnection(void);

    void updateRenderObjectsDebug(float dtime); // update of render actors debug draw information, will be called while the simulation is NOT running
    void updateRenderObjectsSync(float dtime);  // update of render objects while the simulation is NOT running (for particles, cloth etc. because data is not double buffered)
    void updateRenderObjectsAsync(float dtime); // update of render objects, potentially while the simulation is running (for rigid bodies etc. because data is double buffered)

    Stepper* getStepper(void);

    void setScratchBlockSize(size_t size);

    void onDebugDrawChange(bool enabled);
    bool initDebugRenderer(void);

private:
    void setupDefaultRigidDynamic(physx::PxRigidDynamic& body, bool kinematic = false);
    void setupDefaultRigidStatic(physx::PxRigidStatic& body);

    X_INLINE bool IsPaused(void) const;
    X_INLINE void togglePause(void);

    X_INLINE void setSubStepper(const float32_t stepSize, const uint32_t maxSteps);

private:
    void cmd_TogglePvd(core::IConsoleCmdArgs* pArgs);
    void cmd_TogglePause(core::IConsoleCmdArgs* pArgs);
    void cmd_StepOne(core::IConsoleCmdArgs* pArgs);
    void cmd_ToggleVis(core::IConsoleCmdArgs* pArgs);
    void cmd_SetAllScales(core::IConsoleCmdArgs* pArgs);

private:
    core::MemoryArenaBase* arena_;

    PhysxCpuDispacher jobDispatcher_;
    PhysxArenaAllocator allocator_;
    PhysxLogger logger_;

    physx::PxFoundation* pFoundation_;
    physx::PxProfileZoneManager* pProfileZoneManager_;

    physx::PxPhysics* pPhysics_;
    physx::PxMaterial* pMaterial_;
    physx::PxDefaultCpuDispatcher* pCpuDispatcher_;
    PhysCooking* pCooking_;
    ActiveSceneListArr activeScenes_;

#if PHYSX_SCENE_REQUIRES_LOCK
    core::CriticalSection outofBoundsCS_;
#endif // !PHYSX_SCENE_REQUIRES_LOCK
    ActorsArr outOfBoundsObjects_;

    uint8_t* pScratchBlock_;
    size_t scratchBlockSize_;

    bool waitForResults_;
    bool pause_;
    bool oneFrameUpdate_;
    bool _pad;

    physx::PxTolerancesScale scale_;

    // Steppers
    StepperType::Enum stepperType_;
    DebugStepper debugStepper_;
    FixedStepper fixedStepper_;
    InvertedFixedStepper invertedFixedStepper_;
    VariableStepper variableStepper_;

    PhysXVars vars_;
    SceneListArr scenes_;

    DebugRender* pDebugRender_;
};

X_NAMESPACE_END

#include "XPhysics.inl"