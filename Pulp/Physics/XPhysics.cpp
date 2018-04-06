#include "stdafx.h"
#include "XPhysics.h"
#include "ConverterModule/Cooking.h"
#include "Util/MathHelpers.h"
#include "Util/AssertHandler.h"
#include "Util/DelayLoadHook.h"
#include "Util/FilterShader.h"
#include "JointWrapper.h"
#include "XScene.h"

#include <IConsole.h>
#include <I3DEngine.h>

#include <Hashing\Fnva1Hash.h>
#include <Util\UniquePointer.h>

#include <pvd\PxVisualDebugger.h>
#include <common\windows\PxWindowsDelayLoadHook.h>

// don't change this if you want to load say release physx in a debug build
// just do it via 'gDelayLoadHook.forceConfig'
#if X_DEBUG
#define PHYS_LIB_SUFFIX "DEBUG"
#elif X_RELEASE
#define PHYS_LIB_SUFFIX "PROFILE"
#elif X_SUPER
#define PHYS_LIB_SUFFIX "" // no suffix.
#else
#error "Unknown config"
#endif

X_LINK_LIB("PhysX" PHYS_LIB_SUFFIX);
X_LINK_LIB("PhysXCommon" PHYS_LIB_SUFFIX);
X_LINK_LIB("PhysXCooking" PHYS_LIB_SUFFIX);
X_LINK_LIB("PhysXCharacterKinematic" PHYS_LIB_SUFFIX);

X_LINK_LIB("PhysXExtensions" PHYS_LIB_SUFFIX);
X_LINK_LIB("PhysXVehicle" PHYS_LIB_SUFFIX);

X_LINK_LIB("PhysXProfileSDK" PHYS_LIB_SUFFIX);
X_LINK_LIB("PhysXVisualDebuggerSDK" PHYS_LIB_SUFFIX);

X_LINK_LIB("SimulationController" PHYS_LIB_SUFFIX);
X_LINK_LIB("LowLevel" PHYS_LIB_SUFFIX);
X_LINK_LIB("SceneQuery" PHYS_LIB_SUFFIX);
X_LINK_LIB("PvdRuntime" PHYS_LIB_SUFFIX);

X_NAMESPACE_BEGIN(physics)

namespace
{
#if PHYSX_DEFAULT_ALLOCATOR
    physx::PxDefaultAllocator gDefaultAllocatorCallback;
#endif // !PHYSX_DEFAULT_ALLOCATOR

    void copyToleranceScale(physx::PxTolerancesScale& pxScale, const ToleranceScale& scale)
    {
        pxScale.length = scale.length;
        pxScale.mass = scale.mass;
        pxScale.speed = scale.speed;
    }

    void copySceneDesc(physx::PxSceneLimits& pxLimits, const SceneLimits& limits)
    {
        pxLimits.maxNbActors = limits.maxActors;
        pxLimits.maxNbActors = limits.maxBodies;
        pxLimits.maxNbActors = limits.maxStaticShapes;
        pxLimits.maxNbActors = limits.maxDynamicShapes;
        pxLimits.maxNbActors = limits.maxAggregates;
        pxLimits.maxNbActors = limits.maxConstraints;
        pxLimits.maxNbActors = limits.maxRegions;
        pxLimits.maxNbActors = limits.maxObjectsPerRegion;
    }

    void copySceneDesc(physx::PxSceneDesc& pxSceneDesc, const physx::PxTolerancesScale scale, const SceneDesc& sceneDesc)
    {
        copySceneDesc(pxSceneDesc.limits, sceneDesc.sceneLimitHint);

        pxSceneDesc.gravity = Px3FromVec3(sceneDesc.gravity);
        pxSceneDesc.frictionOffsetThreshold = sceneDesc.frictionOffsetThreshold * scale.length;
        pxSceneDesc.contactCorrelationDistance = sceneDesc.contractCorrelationDis * scale.length;
        pxSceneDesc.bounceThresholdVelocity = sceneDesc.bounceThresholdVelocity * scale.speed;
        pxSceneDesc.sanityBounds = PxBounds3FromAABB(sceneDesc.sanityBounds);

        if (sceneDesc.frictionType == FrictionType::Patch) {
            pxSceneDesc.frictionType = physx::PxFrictionType::ePATCH;
        }
        else if (sceneDesc.frictionType == FrictionType::OneDirectional) {
            pxSceneDesc.frictionType = physx::PxFrictionType::eONE_DIRECTIONAL;
        }
        else if (sceneDesc.frictionType == FrictionType::TwoDirectional) {
            pxSceneDesc.frictionType = physx::PxFrictionType::eTWO_DIRECTIONAL;
        }
        else {
            X_ASSERT_UNREACHABLE();
        }
    }

} // namespace

// ---------------------------------

const physx::PxShapeFlags XPhysics::DEFALT_SHAPE_FLAGS = physx::PxShapeFlag::eVISUALIZATION | physx::PxShapeFlag::eSCENE_QUERY_SHAPE | physx::PxShapeFlag::eSIMULATION_SHAPE;

XPhysics::XPhysics(uint32_t maxSubSteps, core::V2::JobSystem* pJobSys, core::MemoryArenaBase* arena) :
    arena_(arena),
    jobDispatcher_(*pJobSys),
    allocator_(arena),
    pFoundation_(nullptr),
    pProfileZoneManager_(nullptr),
    pPhysics_(nullptr),
    pCooking_(nullptr),
    pMaterial_(nullptr),
    pCpuDispatcher_(nullptr),
    pScratchBlock_(nullptr),
    scratchBlockSize_(0),
    waitForResults_(false),
    pause_(false),
    oneFrameUpdate_(false),
    stepperType_(StepperType::FIXED_STEPPER),
    outOfBoundsObjects_(arena),

    debugStepper_(0.016666660f),
    fixedStepper_(0.016666660f, maxSubSteps),
    invertedFixedStepper_(0.016666660f, maxSubSteps),
    variableStepper_(1.0f / 80.0f, 1.0f / 40.0f, maxSubSteps),

    pDebugRender_(nullptr)

{
    X_ASSERT_NOT_NULL(arena);

    debugStepper_.setHandler(this);
    fixedStepper_.setHandler(this);
    invertedFixedStepper_.setHandler(this);
    variableStepper_.setHandler(this);
}

XPhysics::~XPhysics()
{
    X_DELETE_ARRAY(pScratchBlock_, g_PhysicsArena);
}

// IPhysics
void XPhysics::registerVars(void)
{
    PhysXVars::DebugDrawEnabledDel del;
    del.Bind<XPhysics, &XPhysics::onDebugDrawChange>(this);

    vars_.SetDebugDrawChangedDel(del);
    vars_.RegisterVars();
}

void XPhysics::registerCmds(void)
{
    ADD_COMMAND_MEMBER("phys_toggle_pvd", this, XPhysics, &XPhysics::cmd_TogglePvd, core::VarFlag::SYSTEM,
        "Toggles PVD connection");

    ADD_COMMAND_MEMBER("phys_toggle_pause", this, XPhysics, &XPhysics::cmd_TogglePause, core::VarFlag::SYSTEM,
        "Toggles pausing physics simulation");

    ADD_COMMAND_MEMBER("phys_step_one", this, XPhysics, &XPhysics::cmd_StepOne, core::VarFlag::SYSTEM,
        "Steps one frame in the simulation");

    ADD_COMMAND_MEMBER("phys_toggle_visualization", this, XPhysics, &XPhysics::cmd_ToggleVis, core::VarFlag::SYSTEM,
        "Toggles physics visualization, requires 'phys_draw_debug_scale' to be none negative");

    ADD_COMMAND_MEMBER("phys_draw_debug_scale_set_all", this, XPhysics, &XPhysics::cmd_SetAllScales, core::VarFlag::SYSTEM,
        "Sets all scales values to the specified value. <scale>");
}

bool XPhysics::init(const ToleranceScale& scale)
{
    X_LOG0("PhysicsSys", "Starting");
    X_PROFILE_NO_HISTORY_BEGIN("PhysicsInit", core::profiler::SubSys::PHYSICS);

#if X_DEBUG
    gDelayLoadHook.forceConfig(DelayLoadHook::Config::Checked);
#elif X_RELEASE
    gDelayLoadHook.forceConfig(DelayLoadHook::Config::Profile);
#elif X_SUPER
    gDelayLoadHook.forceConfig(DelayLoadHook::Config::Release);
#else
#error "Unknown config"
#endif

    {
        PhysXVars::StrBuf buf;
        const char* pDllOverrideStr = vars_.getDllOverrideStr(buf);
        const size_t len = core::strUtil::strlen(pDllOverrideStr);

        // i might move this str to enum logic into PhysicsVars.cpp
        // just dpeends if i wanna include the delay load def in there.
        if (len) {
            using namespace core::Hash::Literals;
            switch (core::Hash::Fnv1aHash(pDllOverrideStr, len)) {
                case "none"_fnv1a:
                    //	don't override what was set above.
                    //	gDelayLoadHook.forceConfig(DelayLoadHook::Config::Normal);
                    break;
                case "debug"_fnv1a:
                    gDelayLoadHook.forceConfig(DelayLoadHook::Config::Debug);
                    break;
                case "checked"_fnv1a:
                    gDelayLoadHook.forceConfig(DelayLoadHook::Config::Checked);
                    break;
                case "profile"_fnv1a:
                    gDelayLoadHook.forceConfig(DelayLoadHook::Config::Profile);
                    break;
                case "release"_fnv1a:
                    gDelayLoadHook.forceConfig(DelayLoadHook::Config::Release);
                    break;
                default:
                    X_WARNING("Physics", "Invalid dll ovverride value: \"%s\"", pDllOverrideStr);
                    break;
            }
        }
    }

#if X_DEBUG

    // we can't load a release version in debug build since physx removes some symbols in release builds.
    // so we get unresolved procs, we would need to compile release with them in to support it, which is possible..
    if (gDelayLoadHook.getConfig() == DelayLoadHook::Config::Profile || gDelayLoadHook.getConfig() == DelayLoadHook::Config::Release) {
        gDelayLoadHook.forceConfig(DelayLoadHook::Config::Checked);
        X_WARNING("Physics", "Can't load profile or release phyiscs in debug builds, loading checked instead");
    }

#elif X_RELEASE || X_SUPER

    if (gDelayLoadHook.getConfig() == DelayLoadHook::Config::Debug || gDelayLoadHook.getConfig() == DelayLoadHook::Config::Checked) {
        gDelayLoadHook.forceConfig(DelayLoadHook::Config::Profile);
        X_WARNING("Physics", "Can't load debug or checked phyiscs in release builds, loading profile instead");
    }

#endif //

    physx::PxSetAssertHandler(gAssetHandler);
    physx::PxSetPhysXDelayLoadHook(&gDelayLoadHook);
    physx::PxSetPhysXCookingDelayLoadHook(&gDelayLoadHook);

    pFoundation_ = PxCreateFoundation(PX_PHYSICS_VERSION,
#if PHYSX_DEFAULT_ALLOCATOR
        gDefaultAllocatorCallback,
#else
        allocator_,
#endif // !PHYSX_DEFAULT_ALLOCATOR
        logger_);

    if (!pFoundation_) {
        X_ERROR("Physics", "Failed to create foundation");
        return false;
    }

#if !X_SUPER
    pProfileZoneManager_ = &physx::PxProfileZoneManager::createProfileZoneManager(pFoundation_);
    if (!pProfileZoneManager_) {
        X_ERROR("Physics", "Failed to create profile zone manager");
        return false;
    }
#endif // !X_SUPER

    copyToleranceScale(scale_, scale);
    if (!scale_.isValid()) {
        X_ERROR("PhysicsSys", "Scene scale description is invalid");
        return false;
    }

    pPhysics_ = PxCreatePhysics(PX_PHYSICS_VERSION, *pFoundation_,
        scale_, vars_.enableAllocTracking(), pProfileZoneManager_);

    if (!pPhysics_) {
        X_ERROR("Physics", "PxCreatePhysics failed!");
        return false;
    }

    if (!PxInitExtensions(*pPhysics_)) {
        X_ERROR("Physics", "Failed to init extensions");
        return false;
    }

    if (vars_.UnifiedHeightFieldsEnabled()) {
        PxRegisterUnifiedHeightFields(*pPhysics_);
        X_LOG2("Physics", "Enabling unified height fields");
    }

    pCooking_ = X_NEW(PhysCooking, arena_, "RuntimePhysCooking")(arena_);
    if (!pCooking_) {
        X_ERROR("Physics", "CreateCooking failed!");
        return false;
    }

    if (!pCooking_->init(scale_, *pFoundation_, CookingMode::Slow)) {
        X_ERROR("Physics", "Cooking init failed!");
        return false;
    }

    PvdSetup();

    pPhysics_->registerDeletionListener(*this, physx::PxDeletionEventFlag::eUSER_RELEASE);

    pMaterial_ = pPhysics_->createMaterial(0.5f, 0.5f, 0.90f);
    if (!pMaterial_) {
        X_ERROR("Physics", "Failed to create material");
        return false;
    }

    stepperType_ = vars_.getStepperType();
    setScratchBlockSize(vars_.scratchBufferSize());

    // it might be init already.
    onDebugDrawChange(vars_.DebugDrawEnabled() != 0);

    return true;
}

void XPhysics::shutDown(void)
{
    X_LOG0("PhysicsSys", "Shutting Down");

    if (pDebugRender_) {
        X_DELETE(pDebugRender_, arena_);
    }

    debugStepper_.shutdown();
    fixedStepper_.shutdown();
    invertedFixedStepper_.shutdown();
    variableStepper_.shutdown();

    if (pPhysics_) {
        pPhysics_->unregisterDeletionListener(*this);
    }

    for (auto* pScene : scenes_) {
        X_DELETE(pScene, arena_);
    }

    core::SafeRelease(pCpuDispatcher_);
    core::SafeRelease(pMaterial_);

    if (pCooking_) {
        X_DELETE(pCooking_, arena_);
    }

    if (pPhysics_) {
        PvdCleanup();
        PxCloseExtensions();
    }

    core::SafeRelease(pPhysics_);
    core::SafeRelease(pProfileZoneManager_);
    core::SafeRelease(pFoundation_);
}

void XPhysics::release(void)
{
    X_DELETE(this, g_PhysicsArena);
}

void XPhysics::onTickPreRender(core::TimeVal dtime, const AABB& debugVisCullBounds)
{
    stepperType_ = vars_.getStepperType();

    if (oneFrameUpdate_) {
        pause_ = false;
    }

    if (!IsPaused() && activeScenes_.isNotEmpty()) {
        if (vars_.DebugDrawCullEnabled()) {
            for (auto pScene : activeScenes_) {
                pScene->setVisualizationCullingBox(debugVisCullBounds);
            }
        }

        XScene* pScene = activeScenes_.front();
        Stepper* pStepper = getStepper();

        if (outOfBoundsObjects_.isNotEmpty()) {
#if PHYSX_SCENE_REQUIRES_LOCK
            core::ScopedLock<decltype(outofBoundsCS_)> lock(outofBoundsCS_);

#endif // !PHYSX_SCENE_REQUIRES_LOCK
            auto handle = pScene->lock(true);

            for (auto* pActor : outOfBoundsObjects_) {
                pActor->release();
            }
            outOfBoundsObjects_.clear();

            pScene->unLock(handle);
        }

        waitForResults_ = false;

        waitForResults_ = pStepper->advance(pScene->getPxScene(), dtime.GetSeconds(), pScratchBlock_,
            safe_static_cast<uint32_t, size_t>(scratchBlockSize_));

        // tells the stepper shape data is not going to be accessed until next frame
        // (frame ends with stepper->wait(mScene))
        pStepper->renderDone();
    }
}

void XPhysics::onTickPostRender(core::TimeVal dtime)
{
    if (!IsPaused() && waitForResults_ && activeScenes_.isNotEmpty()) {
        XScene* pScene = activeScenes_.front();
        Stepper* pStepper = getStepper();
        pStepper->wait(pScene->getPxScene());

        core::TimeVal simTime = pStepper->getSimulationTime();

        size_t numTransforms = 0;
        pScene->getActiveTransforms(numTransforms);

        X_LOG0_EVERY_N(30, "Phys", "Sim time: %gms transforms: %" PRIuS, simTime.GetMilliSeconds(), numTransforms);

        if (stepperType_ == StepperType::INVERTED_FIXED_STEPPER) {
            pStepper->postRender(dtime.GetSeconds());
        }
    }

    // debug Vis
    if (vars_.DebugDrawEnabled() && pDebugRender_) {
        pDebugRender_->clear();
        // draw debug objects for this scene
        for (auto pScene : activeScenes_) {
            pScene->drawDebug(pDebugRender_);
        }
    }

    // pause if one frame update.
    if (oneFrameUpdate_) {
        oneFrameUpdate_ = false;
        if (!IsPaused()) {
            togglePause();
        }
    }
}

void XPhysics::render(void)
{
    if (vars_.DebugDrawEnabled() && pDebugRender_) {
        pDebugRender_->queueForRender();
    }
}

// ------------------------------------------

IPhysicsCooking* XPhysics::getCooking(void)
{
    return pCooking_;
}

// ------------------------------------------

IScene* XPhysics::createScene(const SceneDesc& desc)
{
    if (!pPhysics_) {
        X_ERROR("Phys", "Failed to create scene, physics system has not be setup");
        return nullptr;
    }

    if (scenes_.size() == scenes_.capacity()) {
        X_ERROR("Phys", "Reached max scene count");
        return nullptr;
    }

    physx::PxSceneDesc sceneDesc(scale_);
    sceneDesc.broadPhaseCallback = this;
    sceneDesc.simulationEventCallback = nullptr;
#if 1
    sceneDesc.filterShader = filter::FilterShader;
#else
    sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
#endif
    sceneDesc.cpuDispatcher = &jobDispatcher_;
    sceneDesc.solverBatchSize = 32; // i might make this lower to improve scaling.
    sceneDesc.nbContactDataBlocks = 256;
    sceneDesc.maxNbContactDataBlocks = 65536;

    // algorithums
    sceneDesc.broadPhaseType = physx::PxBroadPhaseType::eMBP;
    sceneDesc.staticStructure = physx::PxPruningStructure::eSTATIC_AABB_TREE;
    sceneDesc.dynamicStructure = physx::PxPruningStructure::eDYNAMIC_AABB_TREE;
    sceneDesc.dynamicTreeRebuildRateHint = 100;

    if (stepperType_ == StepperType::INVERTED_FIXED_STEPPER) {
        sceneDesc.simulationOrder = physx::PxSimulationOrder::eSOLVE_COLLIDE;
    }
    else {
        sceneDesc.simulationOrder = physx::PxSimulationOrder::eCOLLIDE_SOLVE;
    }

    // flags
    //sceneDesc.flags |= physx::PxSceneFlag::eENABLE_TWO_DIRECTIONAL_FRICTION;
    //sceneDesc.flags |= physx::PxSceneFlag::eENABLE_PCM;
    //sceneDesc.flags |= physx::PxSceneFlag::eENABLE_ONE_DIRECTIONAL_FRICTION;
    //sceneDesc.flags |= physx::PxSceneFlag::eADAPTIVE_FORCE;
    //sceneDesc.flags |= physx::PxSceneFlag::eDISABLE_CONTACT_CACHE;

    // enables populating of getActiveTransforms()
    sceneDesc.flags |= physx::PxSceneFlag::eENABLE_ACTIVETRANSFORMS;

#if PHYSX_SCENE_REQUIRES_LOCK
    sceneDesc.flags |= physx::PxSceneFlag::eREQUIRE_RW_LOCK;
#endif // !PHYSX_SCENE_REQUIRES_LOCK

    // stuff we allow to be configured.
    // keep in mind it may override what you set above.
    copySceneDesc(sceneDesc, scale_, desc);

    auto scene = core::makeUnique<XScene>(arena_, vars_, pPhysics_, arena_);
    if (!scene->createPxScene(sceneDesc)) {
        X_ERROR("Phys", "Failed to create scene");
        return nullptr;
    }

    scenes_.push_back(scene.get());
    return scene.release();
}

void XPhysics::addSceneToSim(IScene* pScene_)
{
    if (activeScenes_.size() == activeScenes_.capacity()) {
        X_ERROR("Phys", "Reached max active scene count");
        return;
    }

    XScene* pScene = static_cast<XScene*>(pScene_);

    auto idx = activeScenes_.find(pScene);
    if (idx == decltype(scenes_)::invalid_index) {
        vars_.SetScene(pScene->getPxScene());

        activeScenes_.push_back(pScene);
    }
}

bool XPhysics::removeSceneFromSim(IScene* pScene_)
{
    XScene* pScene = static_cast<XScene*>(pScene_);

    auto idx = scenes_.find(pScene);
    if (idx != decltype(scenes_)::invalid_index) {
        vars_.ClearScene();
        return activeScenes_.removeIndex(idx);
    }

    return false;
}

void XPhysics::releaseScene(IScene* pScene_)
{
    if (pScene_) {
        // remove from sim if present.
        removeSceneFromSim(pScene_);

        XScene* pScene = static_cast<XScene*>(pScene_);
        auto idx = scenes_.find(pScene);
        scenes_.removeIndex(idx);
        X_DELETE(pScene, arena_);
    }
}

// ------------------------------------------

MaterialHandle XPhysics::createMaterial(MaterialDesc& desc)
{
    auto* pMaterial = pPhysics_->createMaterial(desc.staticFriction, desc.dynamicFriction, desc.restitutio);

    return reinterpret_cast<MaterialHandle>(pMaterial);
}

MaterialHandle XPhysics::getDefaultMaterial(void)
{
    return reinterpret_cast<MaterialHandle>(pMaterial_);
}

// ------------------------------------------

AggregateHandle XPhysics::createAggregate(uint32_t maxActors, bool selfCollisions)
{
    physx::PxAggregate* pAggregate = pPhysics_->createAggregate(maxActors, selfCollisions);

    if (!pAggregate) {
        X_ERROR("Phys", "Failed to create aggregate. maxActor: %" PRIu32 " selfCol: %" PRIu8, maxActors, selfCollisions);
    }

    return reinterpret_cast<AggregateHandle>(pAggregate);
}

bool XPhysics::addActorToAggregate(AggregateHandle handle, ActorHandle actor)
{
    physx::PxAggregate* pAggregate = reinterpret_cast<physx::PxAggregate*>(handle);
    physx::PxActor* pActor = reinterpret_cast<physx::PxActor*>(actor);

    if (!pAggregate->addActor(*pActor)) {
        X_ERROR("Phys", "Failed to add actor %p to aggregate: %p", pActor, pAggregate);
        return false;
    }

    return true;
}

bool XPhysics::releaseAggregate(AggregateHandle handle)
{
    physx::PxAggregate* pAggregate = reinterpret_cast<physx::PxAggregate*>(handle);
    pAggregate->release();
    return true;
}

// ------------------------------------------

IJoint* XPhysics::createJoint(JointType::Enum type, ActorHandle actor0, ActorHandle actor1,
    const Transformf& localFrame0, const Transformf& localFrame1)
{
    physx::PxRigidActor* pActor0 = reinterpret_cast<physx::PxRigidActor*>(actor0);
    physx::PxRigidActor* pActor1 = reinterpret_cast<physx::PxRigidActor*>(actor1);

    const physx::PxTransform& trans0 = PxTransFromQuatTrans(localFrame0);
    const physx::PxTransform& trans1 = PxTransFromQuatTrans(localFrame1);

    // i want to make a api for creating all the diffrent joint types.
    // the problem is how to make it sexy yet functional.
    // i think i will want to update joint info post creation
    // so exposing a api seams like the most sensible thing todo.
    // so lets define some interfaces 1st.
    //
    // ok so i have interfaces for all the diffrent joint types
    // just need to make impl's for the interfaces now so that we can return them.
    static_assert(JointType::ENUM_COUNT == 5, "Added additional JointTypes? this code needs updating.");

    switch (type) {
        case JointType::Fixed: {
            physx::PxFixedJoint* pJoint = physx::PxFixedJointCreate(*pPhysics_, pActor0, trans0, pActor1, trans1);

            return X_NEW(XFixedJoint, arena_, "FixedJoint")(pJoint);
        }
        case JointType::Distance: {
            physx::PxDistanceJoint* pJoint = physx::PxDistanceJointCreate(*pPhysics_, pActor0, trans0, pActor1, trans1);

            return X_NEW(XDistanceJoint, arena_, "DistanceJoint")(pJoint);
        }
        case JointType::Spherical: {
            physx::PxSphericalJoint* pJoint = physx::PxSphericalJointCreate(*pPhysics_, pActor0, trans0, pActor1, trans1);

            return X_NEW(XSphericalJoint, arena_, "SphericalJoint")(pJoint);
        }
        case JointType::Revolute: {
            physx::PxRevoluteJoint* pJoint = physx::PxRevoluteJointCreate(*pPhysics_, pActor0, trans0, pActor1, trans1);

            return X_NEW(XRevoluteJoint, arena_, "RevoluteJoint")(pJoint);
        }
        case JointType::Prismatic: {
            physx::PxPrismaticJoint* pJoint = physx::PxPrismaticJointCreate(*pPhysics_, pActor0, trans0, pActor1, trans1);

            return X_NEW(XPrismaticJoint, arena_, "PrismaticJoint")(pJoint);
        }
    }

    X_ASSERT_UNREACHABLE();
    return nullptr;
}

void XPhysics::releaseJoint(IJoint* pJoint)
{
    X_DELETE(pJoint, arena_);
}

// ------------------------------------------

void XPhysics::setActorDebugNamePointer(ActorHandle handle, const char* pNamePointer)
{
    physx::PxRigidActor& actor = *reinterpret_cast<physx::PxRigidActor*>(handle);

    // should we lock here?
    actor.setName(pNamePointer);
}

void XPhysics::setActorDominanceGroup(ActorHandle handle, int8_t group)
{
    physx::PxRigidActor& actor = *reinterpret_cast<physx::PxRigidActor*>(handle);

    // should we lock here?
    actor.setDominanceGroup(group);
}

// ------------------------------------------

void XPhysics::setGroup(ActorHandle handle, const GroupFlag::Enum group)
{
    physx::PxRigidActor& actor = *reinterpret_cast<physx::PxRigidActor*>(handle);

    filter::SetGroup(actor, group);
}

void XPhysics::setGroupFlags(ActorHandle handle, const GroupFlags groupFlags)
{
    physx::PxRigidActor& actor = *reinterpret_cast<physx::PxRigidActor*>(handle);

    filter::SetGroupMask(actor, groupFlags);
}

// group collision
bool XPhysics::GetGroupCollisionFlag(const GroupFlag::Enum group1, const GroupFlag::Enum group2)
{
    return filter::GetGroupCollisionFlag(group1, group2);
}

void XPhysics::SetGroupCollisionFlag(const GroupFlag::Enum group1, const GroupFlag::Enum group2, const bool enable)
{
    filter::SetGroupCollisionFlag(group1, group2, enable);
}

// ------------------------------------------

TriMeshHandle XPhysics::createTriangleMesh(const DataArr& cooked)
{
    physx::PxDefaultMemoryInputData input(const_cast<physx::PxU8*>(cooked.data()), safe_static_cast<physx::PxU32>(cooked.size()));
    physx::PxTriangleMesh* pTriMesh = pPhysics_->createTriangleMesh(input);

    X_ASSERT_NOT_NULL(pTriMesh);

    return reinterpret_cast<TriMeshHandle>(pTriMesh);
}

ConvexMeshHandle XPhysics::createConvexMesh(const DataArr& cooked)
{
    physx::PxDefaultMemoryInputData input(const_cast<physx::PxU8*>(cooked.data()), safe_static_cast<physx::PxU32>(cooked.size()));
    physx::PxConvexMesh* pConvexMesh = pPhysics_->createConvexMesh(input);

    return reinterpret_cast<ConvexMeshHandle>(pConvexMesh);
}

ConvexMeshHandle XPhysics::createConvexMesh(const uint8_t* pData, size_t length)
{
    physx::PxDefaultMemoryInputData input(const_cast<physx::PxU8*>(pData), safe_static_cast<physx::PxU32>(length));
    physx::PxConvexMesh* pConvexMesh = pPhysics_->createConvexMesh(input);

    return reinterpret_cast<ConvexMeshHandle>(pConvexMesh);
}

HieghtFieldHandle XPhysics::createHieghtField(const DataArr& cooked)
{
    physx::PxDefaultMemoryInputData input(const_cast<physx::PxU8*>(cooked.data()), safe_static_cast<physx::PxU32>(cooked.size()));
    physx::PxHeightField* pHeightField = pPhysics_->createHeightField(input);

    return reinterpret_cast<HieghtFieldHandle>(pHeightField);
}

// ------------------------------------------

ActorHandle XPhysics::createConvexMesh(const Transformf& myTrans, ConvexMeshHandle convex, float density, const Vec3f& scale)
{
    physx::PxConvexMesh* pConvexMesh = X_ASSERT_NOT_NULL(reinterpret_cast<physx::PxConvexMesh*>(convex));

    const physx::PxTransform& trans = PxTransFromQuatTrans(myTrans);
    physx::PxMeshScale meshScale;
    meshScale.rotation = physx::PxQuat::createIdentity();
    meshScale.scale = Px3FromVec3(scale);

    auto* pShape = pPhysics_->createShape(physx::PxConvexMeshGeometry(pConvexMesh, meshScale), *pMaterial_, true, DEFALT_SHAPE_FLAGS);

    physx::PxRigidDynamic* pActor = physx::PxCreateDynamic(*pPhysics_, trans, *pShape, density);

    pShape->release();

    setupDefaultRigidDynamic(*pActor);
    return reinterpret_cast<ActorHandle>(pActor);
}

ActorHandle XPhysics::createTriangleMesh(const Transformf& myTrans, TriMeshHandle tri, float density, const Vec3f& scale)
{
    physx::PxTriangleMesh* pTriMesh = X_ASSERT_NOT_NULL(reinterpret_cast<physx::PxTriangleMesh*>(tri));

    const physx::PxTransform& trans = PxTransFromQuatTrans(myTrans);
    physx::PxMeshScale meshScale;
    meshScale.rotation = physx::PxQuat::createIdentity();
    meshScale.scale = Px3FromVec3(scale);

    auto* pShape = pPhysics_->createShape(physx::PxTriangleMeshGeometry(pTriMesh, meshScale), *pMaterial_, true, DEFALT_SHAPE_FLAGS);

    physx::PxRigidDynamic* pActor = physx::PxCreateDynamic(*pPhysics_, trans, *pShape, density);

    pShape->release();

    setupDefaultRigidDynamic(*pActor);
    return reinterpret_cast<ActorHandle>(pActor);
}

ActorHandle XPhysics::createHieghtField(const Transformf& myTrans, HieghtFieldHandle hf, float density, const Vec3f& heightRowColScale)
{
    physx::PxHeightField* pHeightField = X_ASSERT_NOT_NULL(reinterpret_cast<physx::PxHeightField*>(hf));
    const physx::PxTransform& trans = PxTransFromQuatTrans(myTrans);

    auto* pShape = pPhysics_->createShape(
        physx::PxHeightFieldGeometry(pHeightField, physx::PxMeshGeometryFlags(), heightRowColScale.x, heightRowColScale.y, heightRowColScale.z),
        *pMaterial_,
        true,
        DEFALT_SHAPE_FLAGS);

    physx::PxRigidDynamic* pActor = physx::PxCreateDynamic(*pPhysics_, trans, *pShape, density);

    pShape->release();

    setupDefaultRigidDynamic(*pActor);
    return reinterpret_cast<ActorHandle>(pActor);
}

ActorHandle XPhysics::createSphere(const Transformf& myTrans, float radius, float density)
{
    const physx::PxTransform& trans = PxTransFromQuatTrans(myTrans);

    auto* pShape = pPhysics_->createShape(physx::PxSphereGeometry(radius), *pMaterial_, true, DEFALT_SHAPE_FLAGS);

    physx::PxRigidDynamic* pActor = physx::PxCreateDynamic(*pPhysics_, trans, *pShape, density);

    pShape->release();

    setupDefaultRigidDynamic(*pActor);
    return reinterpret_cast<ActorHandle>(pActor);
}

ActorHandle XPhysics::createCapsule(const Transformf& myTrans, float radius, float halfHeight, float density)
{
    const physx::PxTransform& trans = PxTransFromQuatTrans(myTrans);

    auto* pShape = pPhysics_->createShape(physx::PxCapsuleGeometry(radius, halfHeight), *pMaterial_, true, DEFALT_SHAPE_FLAGS);

    physx::PxRigidDynamic* pActor = physx::PxCreateDynamic(*pPhysics_, trans, *pShape, density);

    pShape->release();

    setupDefaultRigidDynamic(*pActor);
    return reinterpret_cast<ActorHandle>(pActor);
}

ActorHandle XPhysics::createBox(const Transformf& myTrans, const AABB& bounds, float density)
{
    const physx::PxTransform& trans = PxTransFromQuatTrans(myTrans);
    physx::PxBoxGeometry geo(Px3FromVec3(bounds.halfVec()));

    auto* pShape = pPhysics_->createShape(geo, *pMaterial_, true, DEFALT_SHAPE_FLAGS);

    physx::PxRigidDynamic* pActor = physx::PxCreateDynamic(*pPhysics_, trans, *pShape, density);

    pShape->release();

    setupDefaultRigidDynamic(*pActor);
    return reinterpret_cast<ActorHandle>(pActor);
}

// ------------------------------------------

ActorHandle XPhysics::createStaticTriangleMesh(const Transformf& myTrans, TriMeshHandle tri, const Vec3f& scale)
{
    physx::PxTriangleMesh* pTriMesh = X_ASSERT_NOT_NULL(reinterpret_cast<physx::PxTriangleMesh*>(tri));
    const physx::PxTransform& trans = PxTransFromQuatTrans(myTrans);

    physx::PxMeshScale meshScale;
    meshScale.rotation = physx::PxQuat::createIdentity();
    meshScale.scale = Px3FromVec3(scale);

    auto* pShape = pPhysics_->createShape(physx::PxTriangleMeshGeometry(pTriMesh, meshScale), *pMaterial_, true, DEFALT_SHAPE_FLAGS);

    physx::PxRigidStatic* pActor = physx::PxCreateStatic(*pPhysics_, trans, *pShape);

    pShape->release();

    setupDefaultRigidStatic(*pActor);
    return reinterpret_cast<ActorHandle>(pActor);
}

ActorHandle XPhysics::createStaticHieghtField(const Transformf& myTrans, HieghtFieldHandle hf, const Vec3f& heightRowColScale)
{
    physx::PxHeightField* pHeightField = X_ASSERT_NOT_NULL(reinterpret_cast<physx::PxHeightField*>(hf));
    const physx::PxTransform& trans = PxTransFromQuatTrans(myTrans);

    auto* pShape = pPhysics_->createShape(
        physx::PxHeightFieldGeometry(pHeightField, physx::PxMeshGeometryFlags(), heightRowColScale.x, heightRowColScale.y, heightRowColScale.z),
        *pMaterial_,
        true,
        DEFALT_SHAPE_FLAGS);

    physx::PxRigidStatic* pActor = physx::PxCreateStatic(*pPhysics_, trans, *pShape);

    pShape->release();

    setupDefaultRigidStatic(*pActor);
    return reinterpret_cast<ActorHandle>(pActor);
}

ActorHandle XPhysics::createStaticPlane(const Transformf& myTrans)
{
    const physx::PxTransform& trans = PxTransFromQuatTrans(myTrans);

    auto* pShape = pPhysics_->createShape(physx::PxPlaneGeometry(), *pMaterial_, true, DEFALT_SHAPE_FLAGS);

    // calls: createRigidStatic, attachShape
    physx::PxRigidStatic* pActor = physx::PxCreateStatic(*pPhysics_, trans, *pShape);

    pShape->release();

    setupDefaultRigidStatic(*pActor);
    return reinterpret_cast<ActorHandle>(pActor);
}

ActorHandle XPhysics::createStaticSphere(const Transformf& myTrans, float radius)
{
    const physx::PxTransform& trans = PxTransFromQuatTrans(myTrans);

    auto* pShape = pPhysics_->createShape(physx::PxSphereGeometry(radius), *pMaterial_, true, DEFALT_SHAPE_FLAGS);

    // calls: createRigidStatic, attachShape
    physx::PxRigidStatic* pActor = physx::PxCreateStatic(*pPhysics_, trans, *pShape);

    pShape->release();

    setupDefaultRigidStatic(*pActor);
    return reinterpret_cast<ActorHandle>(pActor);
}

ActorHandle XPhysics::createStaticCapsule(const Transformf& myTrans, float radius, float halfHeight)
{
    const physx::PxTransform& trans = PxTransFromQuatTrans(myTrans);

    auto* pShape = pPhysics_->createShape(physx::PxCapsuleGeometry(radius, halfHeight), *pMaterial_, true, DEFALT_SHAPE_FLAGS);

    // calls: createRigidStatic, attachShape
    physx::PxRigidStatic* pActor = physx::PxCreateStatic(*pPhysics_, trans, *pShape);

    pShape->release();

    setupDefaultRigidStatic(*pActor);
    return reinterpret_cast<ActorHandle>(pActor);
}

ActorHandle XPhysics::createStaticBox(const Transformf& myTrans, const AABB& bounds)
{
    const physx::PxTransform& trans = PxTransFromQuatTrans(myTrans);
    physx::PxBoxGeometry geo(Px3FromVec3(bounds.halfVec()));

    auto* pShape = pPhysics_->createShape(geo, *pMaterial_, true, DEFALT_SHAPE_FLAGS);

    // calls: createRigidStatic, attachShape
    physx::PxRigidStatic* pActor = physx::PxCreateStatic(*pPhysics_, trans, *pShape);

    pShape->release();

    setupDefaultRigidStatic(*pActor);
    return reinterpret_cast<ActorHandle>(pActor);
}

ActorHandle XPhysics::createStaticTrigger(const Transformf& myTrans, const AABB& bounds)
{
    const physx::PxTransform& trans = PxTransFromQuatTrans(myTrans);
    physx::PxBoxGeometry geo(Px3FromVec3(bounds.halfVec()));

    auto* pShape = pPhysics_->createShape(geo, *pMaterial_, true, DEFALT_SHAPE_FLAGS);
    pShape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
    pShape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);

    // calls: createRigidStatic, attachShape
    physx::PxRigidStatic* pTrigger = physx::PxCreateStatic(*pPhysics_, trans, *pShape);

    pShape->release();

    setupDefaultRigidStatic(*pTrigger);
    return reinterpret_cast<ActorHandle>(pTrigger);
}

// ------------------------------------------

ActorHandle XPhysics::createActor(const Transformf& myTrans, bool kinematic, const void* pUserData)
{
    const physx::PxTransform& trans = PxTransFromQuatTrans(myTrans);

    physx::PxRigidDynamic* pActor = pPhysics_->createRigidDynamic(trans);
    if (pActor) {
        pActor->userData = const_cast<void*>(pUserData);
        setupDefaultRigidDynamic(*pActor, kinematic);
    }

    return reinterpret_cast<ActorHandle>(pActor);
}

ActorHandle XPhysics::createStaticActor(const Transformf& myTrans, const void* pUserData)
{
    const physx::PxTransform& trans = PxTransFromQuatTrans(myTrans);

    physx::PxRigidStatic* pActor = pPhysics_->createRigidStatic(trans);
    if (pActor) {
        pActor->userData = const_cast<void*>(pUserData);
        setupDefaultRigidStatic(*pActor);
    }

    return reinterpret_cast<ActorHandle>(pActor);
}

void XPhysics::updateMassAndInertia(ActorHandle handle, float density)
{
    physx::PxActor& actor = *reinterpret_cast<physx::PxActor*>(handle);

    auto* pDynActor = actor.isRigidDynamic();
    X_ASSERT_NOT_NULL(pDynActor);

    physx::PxRigidBodyExt::updateMassAndInertia(*pDynActor, density);
}

// ------------------------------------------

void XPhysics::addTriMesh(ActorHandle handle, TriMeshHandle tri, const Vec3f& scale)
{
    physx::PxRigidActor& actor = *reinterpret_cast<physx::PxRigidActor*>(handle);
    physx::PxTriangleMesh* pTriMesh = reinterpret_cast<physx::PxTriangleMesh*>(tri);

    physx::PxMeshScale meshScale;
    meshScale.rotation = physx::PxQuat::createIdentity();
    meshScale.scale = Px3FromVec3(scale);

    auto* pShape = pPhysics_->createShape(physx::PxTriangleMeshGeometry(pTriMesh, meshScale), *pMaterial_, true, DEFALT_SHAPE_FLAGS);

    actor.attachShape(*pShape);
}

void XPhysics::addConvexMesh(ActorHandle handle, ConvexMeshHandle con, const Vec3f& scale)
{
    physx::PxRigidActor& actor = *reinterpret_cast<physx::PxRigidActor*>(handle);
    physx::PxConvexMesh* pConvexMesh = reinterpret_cast<physx::PxConvexMesh*>(con);

    physx::PxMeshScale meshScale;
    meshScale.rotation = physx::PxQuat::createIdentity();
    meshScale.scale = Px3FromVec3(scale);

    auto* pShape = pPhysics_->createShape(physx::PxConvexMeshGeometry(pConvexMesh, meshScale), *pMaterial_, true, DEFALT_SHAPE_FLAGS);

    actor.attachShape(*pShape);
}

void XPhysics::addHieghtField(ActorHandle handle, HieghtFieldHandle hf, const Vec3f& heightRowColScale)
{
    physx::PxRigidActor& actor = *reinterpret_cast<physx::PxRigidActor*>(handle);
    physx::PxHeightField* pHf = reinterpret_cast<physx::PxHeightField*>(hf);

    auto* pShape = pPhysics_->createShape(
        physx::PxHeightFieldGeometry(pHf, physx::PxMeshGeometryFlags(), heightRowColScale.x, heightRowColScale.y, heightRowColScale.z),
        *pMaterial_,
        true,
        DEFALT_SHAPE_FLAGS);

    actor.attachShape(*pShape);
}

void XPhysics::addBox(ActorHandle handle, const AABB& aabb)
{
    physx::PxRigidActor& actor = *reinterpret_cast<physx::PxRigidActor*>(handle);
    physx::PxBoxGeometry geo(Px3FromVec3(aabb.halfVec()));

    auto* pShape = pPhysics_->createShape(geo, *pMaterial_, true, DEFALT_SHAPE_FLAGS);

    actor.attachShape(*pShape);
}

void XPhysics::addBox(ActorHandle handle, const AABB& aabb, const Vec3f& localPose)
{
    physx::PxRigidActor& actor = *reinterpret_cast<physx::PxRigidActor*>(handle);
    physx::PxBoxGeometry geo(Px3FromVec3(aabb.halfVec()));
    physx::PxTransform trans(Px3FromVec3(localPose));

    auto* pShape = pPhysics_->createShape(geo, *pMaterial_, true, DEFALT_SHAPE_FLAGS);
    pShape->setLocalPose(trans);

    actor.attachShape(*pShape);
}

void XPhysics::addSphere(ActorHandle handle, float radius)
{
    physx::PxRigidActor& actor = *reinterpret_cast<physx::PxRigidActor*>(handle);

    auto* pShape = pPhysics_->createShape(physx::PxSphereGeometry(radius), *pMaterial_, true, DEFALT_SHAPE_FLAGS);

    actor.attachShape(*pShape);
}

void XPhysics::addSphere(ActorHandle handle, float radius, const Vec3f& localPose)
{
    physx::PxRigidActor& actor = *reinterpret_cast<physx::PxRigidActor*>(handle);
    physx::PxTransform trans(Px3FromVec3(localPose));

    auto* pShape = pPhysics_->createShape(physx::PxSphereGeometry(radius), *pMaterial_, true, DEFALT_SHAPE_FLAGS);
    pShape->setLocalPose(trans);

    actor.attachShape(*pShape);
}

void XPhysics::addCapsule(ActorHandle handle, float radius, float halfHeight)
{
    physx::PxRigidActor& actor = *reinterpret_cast<physx::PxRigidActor*>(handle);

    auto* pShape = pPhysics_->createShape(physx::PxCapsuleGeometry(radius, halfHeight), *pMaterial_, true, DEFALT_SHAPE_FLAGS);

    actor.attachShape(*pShape);
}

// ------------------------------------------

void XPhysics::setupDefaultRigidDynamic(physx::PxRigidDynamic& body, bool kinematic)
{
    body.setActorFlag(physx::PxActorFlag::eVISUALIZATION, true);
    body.setAngularDamping(0.5f);

    if (kinematic) {
        body.setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
    }
}

void XPhysics::setupDefaultRigidStatic(physx::PxRigidStatic& body)
{
    body.setActorFlag(physx::PxActorFlag::eVISUALIZATION, true);
    body.setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, true);
}

// --------------------------------------------------

void XPhysics::onPvdSendClassDescriptions(PVD::PvdConnection& conn)
{
    X_UNUSED(conn);
}

void XPhysics::onPvdConnected(PVD::PvdConnection& conn)
{
    X_UNUSED(conn);

    //setup joint visualization.  This gets piped to pvd.
    auto* pVisualDebugger = pPhysics_->getVisualDebugger();
    pVisualDebugger->setVisualizeConstraints(true);
    pVisualDebugger->setVisualDebuggerFlag(physx::PxVisualDebuggerFlag::eTRANSMIT_CONTACTS, true);
    pVisualDebugger->setVisualDebuggerFlag(physx::PxVisualDebuggerFlag::eTRANSMIT_SCENEQUERIES, true);
}

void XPhysics::onPvdDisconnected(PVD::PvdConnection& conn)
{
    conn.release();
}

void XPhysics::onRelease(const physx::PxBase* pObserved, void* pUserData,
    physx::PxDeletionEventFlag::Enum deletionEvent)
{
    X_UNUSED(pUserData);
    X_UNUSED(deletionEvent);

    if (pObserved->is<physx::PxRigidActor>()) {
#if 0
		const physx::PxRigidActor* actor = static_cast<const physx::PxRigidActor*>(pObserved);

		removeRenderActorsFromPhysicsActor(actor);

		auto actorIter = std::find(mPhysicsActors.begin(), mPhysicsActors.end(), actor);
		if (actorIter != mPhysicsActors.end())
		{
			mPhysicsActors.erase(actorIter);
		}
#endif
    }
}

void XPhysics::onObjectOutOfBounds(physx::PxShape& shape, physx::PxActor& actor)
{
    // This function is called when an object leaves the broad - phase.
    // shape	Shape that left the broad - phase bounds
    // actor	Owner actor

    // for now we just log that somthing left the physx world.
    // dunno what shit to log.
    X_ERROR("Phys", "Obbject out of bounds. Name: \"%s\"", actor.getName());

#if PHYSX_SCENE_REQUIRES_LOCK
    core::ScopedLock<decltype(outofBoundsCS_)> lock(outofBoundsCS_);
#endif // !PHYSX_SCENE_REQUIRES_LOCK

    // que it for removal;
    if (outOfBoundsObjects_.find(&actor) == ActorsArr::invalid_index) {
        outOfBoundsObjects_.append(&actor);
    }
}

void XPhysics::onObjectOutOfBounds(physx::PxAggregate& aggregate)
{
    // This function is called when an aggregate leaves the broad - phase.
    // 	An aggregate is a collection of actors.
    X_ERROR("Phys", "Aggregate out of bounds. %p", &aggregate);
}

void XPhysics::onSubstepPreFetchResult(void)
{
}

void XPhysics::onSubstep(float32_t dtTime)
{
    X_UNUSED(dtTime);
}

void XPhysics::onSubstepSetup(float dtime, physx::PxBaseTask* cont)
{
    X_UNUSED(dtime);
    X_UNUSED(cont);
}

// -------------------------------

void XPhysics::PvdSetup(void)
{
    if (!vars_.isPVDEnabled()) {
        return;
    }

    auto* pPVD = pPhysics_->getPvdConnectionManager();
    if (!pPVD) {
        X_WARNING("Physics", "PVD is enabled but loaded modules don't support PVD. Hint try changing: 'phys_dll_override'");
        return;
    }

    pPVD->addHandler(*this);

    // attemp to connect now.
    togglePvdConnection();
}

void XPhysics::PvdCleanup(void)
{
    closePvdConnection();

    auto* pPVD = pPhysics_->getPvdConnectionManager();
    if (pPVD) {
        pPVD->removeHandler(*this);
    }
}

void XPhysics::togglePvdConnection(void)
{
    physx::PxVisualDebuggerConnectionManager* pPVD = pPhysics_->getPvdConnectionManager();
    if (!pPVD) {
        return;
    }

    if (pPVD->isConnected()) {
        pPVD->disconnect();
    }
    else {
        createPvdConnection();
    }
}

void XPhysics::closePvdConnection(void)
{
    physx::PxVisualDebuggerConnectionManager* pPVD = pPhysics_->getPvdConnectionManager();
    if (!pPVD) {
        return;
    }

    if (pPVD->isConnected()) {
        pPVD->disconnect();
    }
}

void XPhysics::createPvdConnection(void)
{
    physx::PxVisualDebuggerConnectionManager* pPVD = pPhysics_->getPvdConnectionManager();
    if (!pPVD) {
        X_ERROR("PhysicsSys", "Failed to get PVD connection manager");
        return;
    }

    X_ASSERT(!pPVD->isConnected(), "Already connected")();

    //The connection flags state overall what data is to be sent to PVD.  Currently
    //the Debug connection flag requires support from the implementation (don't send
    //the data when debug isn't set) but the other two flags, profile and memory
    //are taken care of by the PVD SDK.

    //Use these flags for a clean profile trace with minimal overhead
    const int32_t flags = vars_.getPVDFlags();
    physx::PxVisualDebuggerConnectionFlags connectionFlags(0);

    if (core::bitUtil::IsBitFlagSet(flags, core::bitUtil::AlphaBit('d'))) {
        connectionFlags |= physx::PxVisualDebuggerConnectionFlag::eDEBUG;
    }
    if (core::bitUtil::IsBitFlagSet(flags, core::bitUtil::AlphaBit('p'))) {
        connectionFlags |= physx::PxVisualDebuggerConnectionFlag::ePROFILE;
    }
    if (core::bitUtil::IsBitFlagSet(flags, core::bitUtil::AlphaBit('m'))) {
        connectionFlags |= physx::PxVisualDebuggerConnectionFlag::eMEMORY;
    }

    //Create a pvd connection that writes data straight to the filesystem.  This is
    //the fastest connection on windows for various reasons.  First, the transport is quite fast as
    //pvd writes data in blocks and filesystems work well with that abstraction.
    //Second, you don't have the PVD application parsing data and using CPU and memory bandwidth
    //while your application is running.
    //PxVisualDebuggerExt::createConnection(mPhysics->getPvdConnectionManager(), "c:\\temp.pxd2", theConnectionFlags);

    //The normal way to connect to pvd.  PVD needs to be running at the time this function is called.
    //We don't worry about the return value because we are already registered as a listener for connections
    //and thus our onPvdConnected call will take care of setting up our basic connection state.
    PhysXVars::StrBuf buf;

    physx::PxVisualDebuggerConnection* pCon = physx::PxVisualDebuggerExt::createConnection(
        pPVD,
        vars_.getPVDIp(buf),
        vars_.getPVDPort(),
        vars_.getPVDTimeoutMS(),
        connectionFlags);

    if (pCon && pCon->isConnected()) {
        X_LOG1("PhysicsSys", "Connected to PVD");
    }
    else {
        X_WARNING("PhysicsSys", "Failed to create PVD connection");
    }
}

// --------------------------------------------------------

Stepper* XPhysics::getStepper(void)
{
    switch (stepperType_) {
        case StepperType::DEBUG_STEPPER:
            return &debugStepper_;
        case StepperType::FIXED_STEPPER:
            return &fixedStepper_;
        case StepperType::INVERTED_FIXED_STEPPER:
            return &invertedFixedStepper_;
        case StepperType::VARIABLE_STEPPER:
            return &variableStepper_;
        default:
            return &variableStepper_;
    };
}

void XPhysics::setScratchBlockSize(size_t size)
{
    if (size == scratchBlockSize_) {
        return;
    }

    static const size_t MULTIPLE_OF = 1024 * 16;

    if ((size % MULTIPLE_OF) != 0) {
        X_ERROR("PhysicsSys", "Scratch block must be a multiple of %" PRIuS ". requested size: %" PRIuS, MULTIPLE_OF, size);
        return;
    }

    if (pScratchBlock_) {
        X_DELETE_ARRAY(pScratchBlock_, g_PhysicsArena);
    }

    pScratchBlock_ = X_NEW_ARRAY_ALIGNED(uint8_t, size, g_PhysicsArena, "ScratchBlock", 64);
    scratchBlockSize_ = size;
}

void XPhysics::onDebugDrawChange(bool enabled)
{
    if (enabled && !pDebugRender_) {
        if (!initDebugRenderer()) {
            X_ERROR("Physics", "Failed to init debugdraw");
        }
    }
}

bool XPhysics::initDebugRenderer(void)
{
    X_ASSERT_NOT_NULL(gEnv);
    X_ASSERT_NOT_NULL(gEnv->p3DEngine);

    X_ASSERT(!pDebugRender_, "Debug render already init")(pDebugRender_);

    auto* pPrimCon = gEnv->p3DEngine->getPrimContext(engine::PrimContext::PHYSICS);
    if (!pPrimCon) {
        return false;
    }

    pDebugRender_ = X_NEW(DebugRender, arena_, "PhysDebugRender")(pPrimCon);
    return true;
}

void XPhysics::cmd_TogglePvd(core::IConsoleCmdArgs* pArgs)
{
    X_UNUSED(pArgs);

    togglePvdConnection();
}

void XPhysics::cmd_TogglePause(core::IConsoleCmdArgs* pArgs)
{
    X_UNUSED(pArgs);

    togglePause();
}

void XPhysics::cmd_StepOne(core::IConsoleCmdArgs* pArgs)
{
    X_UNUSED(pArgs);

    oneFrameUpdate_ = true;
}

void XPhysics::cmd_ToggleVis(core::IConsoleCmdArgs* pArgs)
{
    X_UNUSED(pArgs);

    // we want to turn off eScale but we need todo it via vars.
    // otherwise the var value is out of
    vars_.SetDebugDrawEnabled(!vars_.DebugDrawEnabled());
}

void XPhysics::cmd_SetAllScales(core::IConsoleCmdArgs* pArgs)
{
    if (pArgs->GetArgCount() != 2) {
        X_WARNING("Phys", "phys_draw_debug_scale_set_all <scale>");
        return;
    }

    const char* pScaleStr = pArgs->GetArg(1);
    const float scle = core::strUtil::StringToFloat<float>(pScaleStr);

    vars_.SetAllScalesToValue(scle);
}

X_NAMESPACE_END