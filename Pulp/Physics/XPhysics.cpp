#include "stdafx.h"
#include "XPhysics.h"
#include "MathHelpers.h"
#include "Cooking.h"
#include "AssertHandler.h"
#include "DelayLoadHook.h"
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
#define PHYS_LIB_SUFFIX "CHECKED"
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


XPhysics::PvdParameters::PvdParameters() :
	ip("127.0.0.1"),
	port(5425),
	timeout(10),
	useFullPvdConnection(true)
{
}

// ---------------------------------

const physx::PxShapeFlags XPhysics::DEFALT_SHAPE_FLAGS =
	physx::PxShapeFlag::eVISUALIZATION |
	physx::PxShapeFlag::eSCENE_QUERY_SHAPE |
	physx::PxShapeFlag::eSIMULATION_SHAPE;



XPhysics::XPhysics(uint32_t maxSubSteps, core::V2::JobSystem* pJobSys, core::MemoryArenaBase* arena) :
	arena_(arena),
	jobDispatcher_(*pJobSys),
	allocator_(arena),
	pFoundation_(nullptr),
	pProfileZoneManager_(nullptr),
	pPhysics_(nullptr),
	pCooking_(nullptr),
	pScene_(nullptr),
	pMaterial_(nullptr),
	pCpuDispatcher_(nullptr),
	pScratchBlock_(nullptr),
	scratchBlockSize_(0),
	waitForResults_(false),
	pause_(false),
	oneFrameUpdate_(false),
	stepperType_(StepperType::FIXED_STEPPER),

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
		const char* pDllOverrideStr = vars_.getDllOverrideStr();
		const size_t len = core::strUtil::strlen(pDllOverrideStr);

		// i might move this str to enum logic into PhysicsVars.cpp 
		// just dpeends if i wanna include the delay load def in there.
		if (len)
		{
			using namespace core::Hash::Fnva1Literals;
			switch (core::Hash::Fnv1aHash(pDllOverrideStr, len))
			{
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

	physx::PxSetAssertHandler(gAssetHandler);
	physx::PxSetPhysXDelayLoadHook(&gDelayLoadHook);
	physx::PxSetPhysXCookingDelayLoadHook(&gDelayLoadHook);

	pFoundation_ = PxCreateFoundation(PX_PHYSICS_VERSION, 
#if PHYSX_DEFAULT_ALLOCATOR
		gDefaultAllocatorCallback,
#else
		allocator_,
#endif // !PHYSX_DEFAULT_ALLOCATOR
		logger_
	);

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

	bool recordMemoryAllocations = true;

	copyToleranceScale(scale_, scale);
	if (!scale_.isValid()) {
		X_ERROR("PhysicsSys", "Scene scale description is invalid");
		return false;
	}

	pPhysics_ = PxCreatePhysics(PX_PHYSICS_VERSION, *pFoundation_,
		scale_, recordMemoryAllocations, pProfileZoneManager_);

	if (!pPhysics_) {
		X_ERROR("Physics", "PxCreatePhysics failed!");
		return false;
	}	

	if (!PxInitExtensions(*pPhysics_)) {
		X_ERROR("Physics", "Failed to init extensions");
		return false;
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
	
	if (pPhysics_->getPvdConnectionManager()) {
		pPhysics_->getPvdConnectionManager()->addHandler(*this);
	}

	togglePvdConnection();

	pPhysics_->registerDeletionListener(*this, physx::PxDeletionEventFlag::eUSER_RELEASE);


	pMaterial_ = pPhysics_->createMaterial(1.f, 1.f, 1.f);
	if (!pMaterial_) {
		X_ERROR("Physics", "Failed to create material");
		return false;
	}

	stepperType_ = vars_.GetStepperType();
	setScratchBlockSize(vars_.ScratchBufferSize());
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

	if (pScene_) {
		releaseScene(pScene_);
	}

	core::SafeRelease(pCpuDispatcher_);
	core::SafeRelease(pMaterial_);

	if (pCooking_) {
		X_DELETE(pCooking_, arena_);
	}

	PxCloseExtensions();

	core::SafeRelease(pPhysics_);
	core::SafeRelease(pProfileZoneManager_);
	core::SafeRelease(pFoundation_);
}

void XPhysics::release(void)
{
	X_DELETE(this, g_PhysicsArena);
}


void XPhysics::onTickPreRender(float dtime, const AABB& debugVisCullBounds)
{
	stepperType_ = vars_.GetStepperType();

	if (oneFrameUpdate_) {
		pause_ = false;
	}

	if (!IsPaused() && pScene_)
	{
		if (vars_.DebugDrawCullEnabled()) {
			pScene_->setVisualizationCullingBox(debugVisCullBounds);
		}

		Stepper* pStepper = getStepper();

		waitForResults_ = false;


		waitForResults_ = pStepper->advance(pScene_->getPxScene(), dtime, pScratchBlock_,
			safe_static_cast<uint32_t, size_t>(scratchBlockSize_));

		// tells the stepper shape data is not going to be accessed until next frame 
		// (frame ends with stepper->wait(mScene))
		pStepper->renderDone();	
	}
}


void XPhysics::onTickPostRender(float dtime)
{
	if (!pScene_) {
		return;
	}

	if (!IsPaused() && waitForResults_)
	{
		Stepper* pStepper = getStepper();
		pStepper->wait(pScene_->getPxScene());

		core::TimeVal simTime = pStepper->getSimulationTime();

		X_LOG0_EVERY_N(30, "Phys", "Sim time: %gms", simTime.GetMilliSeconds());

		if (stepperType_ == StepperType::INVERTED_FIXED_STEPPER) {
			pStepper->postRender(dtime);
		}
	}

	// debug Vis
	if (vars_.DebugDrawEnabled() && pDebugRender_)
	{
		pDebugRender_->clear();
		// draw debug objects for this scene
		pScene_->drawDebug(pDebugRender_);
	}

	// pause if one frame update.
	if (oneFrameUpdate_)
	{
		oneFrameUpdate_ = false;
		if (!IsPaused()) {
			togglePause();
		}
	}
}

void XPhysics::render(void)
{
	if (vars_.DebugDrawEnabled() && pDebugRender_)
	{
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
	if (pScene_) {
		X_ERROR("Phys", "Only one scene is allowed, release the old before creating a new one");
		return nullptr;
	}

	physx::PxSceneDesc sceneDesc(scale_);
	sceneDesc.broadPhaseCallback = this;
	sceneDesc.simulationEventCallback = this;
	sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
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
	
	// 
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

	vars_.SetScene(scene->getPxScene());

	pScene_ = scene.get();
	return scene.release();
}

void XPhysics::releaseScene(IScene* pScene)
{
	// todo free :D!
	X_ASSERT(pScene == pScene_, "Invalid scene pointer")(pScene, pScene_);

	pScene_ = nullptr;
	X_DELETE(pScene, arena_);
}

// ------------------------------------------

MaterialHandle XPhysics::createMaterial(MaterialDesc& desc)
{
	auto* pMaterial = pPhysics_->createMaterial(desc.staticFriction, desc.dynamicFriction, desc.restitutio);

	return reinterpret_cast<MaterialHandle>(pMaterial);
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
	const QuatTransf& localFrame0, const QuatTransf& localFrame1)
{
	physx::PxRigidActor* pActor0 = reinterpret_cast<physx::PxRigidActor*>(actor0);
	physx::PxRigidActor* pActor1 = reinterpret_cast<physx::PxRigidActor*>(actor1);

	physx::PxTransform trans0 = PxTransFromQuatTrans(localFrame0);
	physx::PxTransform trans1 = PxTransFromQuatTrans(localFrame1);


	// i want to make a api for creating all the diffrent joint types.
	// the problem is how to make it sexy yet functional.
	// i think i will want to update joint info post creation
	// so exposing a api seams like the most sensible thing todo.
	// so lets define some interfaces 1st.
	// 
	// ok so i have interfaces for all the diffrent joint types
	// just need to make impl's for the interfaces now so that we can return them.
	static_assert(JointType::ENUM_COUNT == 5, "Added additional JointTypes? this code needs updating.");

	switch (type)
	{
		case JointType::Fixed:
		{
			physx::PxFixedJoint* pJoint = physx::PxFixedJointCreate(*pPhysics_, pActor0, trans0, pActor1, trans1);

			return X_NEW(XFixedJoint, arena_, "FixedJoint")(pJoint);
		}
		case JointType::Distance:
		{
			physx::PxDistanceJoint* pJoint = physx::PxDistanceJointCreate(*pPhysics_, pActor0, trans0, pActor1, trans1);
		
			return X_NEW(XDistanceJoint, arena_, "DistanceJoint")(pJoint);
		}
		case JointType::Spherical:
		{
			physx::PxSphericalJoint* pJoint = physx::PxSphericalJointCreate(*pPhysics_, pActor0, trans0, pActor1, trans1);

			return X_NEW(XSphericalJoint, arena_, "SphericalJoint")(pJoint);
		}
		case JointType::Revolute:
		{
			physx::PxRevoluteJoint* pJoint = physx::PxRevoluteJointCreate(*pPhysics_, pActor0, trans0, pActor1, trans1);
		
			return X_NEW(XRevoluteJoint, arena_, "RevoluteJoint")(pJoint);
		}
		case JointType::Prismatic:
		{
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

ActorHandle XPhysics::createConvexMesh(const QuatTransf& myTrans, const DataArr& cooked, float density, const Vec3f& scale)
{
	physx::PxTransform trans = PxTransFromQuatTrans(myTrans);

	// Looks liek they forgot const on PxDefaultMemoryInputData constructor, as the member var is a const pointer.
	physx::PxDefaultMemoryInputData input(const_cast<physx::PxU8*>(cooked.data()), safe_static_cast<physx::PxU32>(cooked.size()));
	physx::PxConvexMesh* pConvexMesh = pPhysics_->createConvexMesh(input);
	physx::PxMeshScale meshScale;
	meshScale.rotation = physx::PxQuat::createIdentity();
	meshScale.scale = Px3FromVec3(scale);

	auto* pShape = pPhysics_->createShape(physx::PxConvexMeshGeometry(pConvexMesh, meshScale), *pMaterial_, true, DEFALT_SHAPE_FLAGS);

	physx::PxRigidDynamic* pActor = physx::PxCreateDynamic(*pPhysics_, trans, *pShape, density);

	pShape->release();

	setupDefaultRigidDynamic(*pActor);
	return reinterpret_cast<ActorHandle>(pActor);
}

ActorHandle XPhysics::createTriangleMesh(const QuatTransf& myTrans, const DataArr& cooked, float density, const Vec3f& scale)
{
	physx::PxTransform trans = PxTransFromQuatTrans(myTrans);

	// Looks liek they forgot const on PxDefaultMemoryInputData constructor, as the member var is a const pointer.
	physx::PxDefaultMemoryInputData input(const_cast<physx::PxU8*>(cooked.data()), safe_static_cast<physx::PxU32>(cooked.size()));
	physx::PxTriangleMesh* pTriMesh = pPhysics_->createTriangleMesh(input);
	physx::PxMeshScale meshScale;
	meshScale.rotation = physx::PxQuat::createIdentity();
	meshScale.scale = Px3FromVec3(scale);

	auto* pShape = pPhysics_->createShape(physx::PxTriangleMeshGeometry(pTriMesh, meshScale), *pMaterial_, true, DEFALT_SHAPE_FLAGS);

	physx::PxRigidDynamic* pActor = physx::PxCreateDynamic(*pPhysics_, trans, *pShape, density);

	pShape->release();

	setupDefaultRigidDynamic(*pActor);
	return reinterpret_cast<ActorHandle>(pActor);
}

ActorHandle XPhysics::createHieghtField(const QuatTransf& myTrans, const DataArr& cooked, float density, const Vec3f& heightRowColScale)
{
	physx::PxTransform trans = PxTransFromQuatTrans(myTrans);

	// Looks liek they forgot const on PxDefaultMemoryInputData constructor, as the member var is a const pointer.
	physx::PxDefaultMemoryInputData input(const_cast<physx::PxU8*>(cooked.data()), safe_static_cast<physx::PxU32>(cooked.size()));
	physx::PxHeightField* pHeightField = pPhysics_->createHeightField(input);

	auto* pShape = pPhysics_->createShape(
		physx::PxHeightFieldGeometry(pHeightField, physx::PxMeshGeometryFlags(), heightRowColScale.x, heightRowColScale.y, heightRowColScale.z),
		*pMaterial_, 
		true, 
		DEFALT_SHAPE_FLAGS
	);

	physx::PxRigidDynamic* pActor = physx::PxCreateDynamic(*pPhysics_, trans, *pShape, density);

	pShape->release();

	setupDefaultRigidDynamic(*pActor);
	return reinterpret_cast<ActorHandle>(pActor);
}

ActorHandle XPhysics::createPlane(const QuatTransf& myTrans, float density)
{
	physx::PxTransform trans = PxTransFromQuatTrans(myTrans);

	auto* pShape = pPhysics_->createShape(physx::PxPlaneGeometry(), *pMaterial_, true, DEFALT_SHAPE_FLAGS);

	physx::PxRigidDynamic* pActor = physx::PxCreateDynamic(*pPhysics_, trans, *pShape, density);

	pShape->release();

	setupDefaultRigidDynamic(*pActor);
	return reinterpret_cast<ActorHandle>(pActor);
}

ActorHandle XPhysics::createSphere(const QuatTransf& myTrans, float radius, float density)
{
	physx::PxTransform trans = PxTransFromQuatTrans(myTrans);

	auto* pShape = pPhysics_->createShape(physx::PxSphereGeometry(radius), *pMaterial_, true, DEFALT_SHAPE_FLAGS);

	physx::PxRigidDynamic* pActor = physx::PxCreateDynamic(*pPhysics_, trans, *pShape, density);

	pShape->release();

	setupDefaultRigidDynamic(*pActor);
	return reinterpret_cast<ActorHandle>(pActor);
}

ActorHandle XPhysics::createCapsule(const QuatTransf& myTrans, float radius, float halfHeight, float density)
{
	physx::PxTransform trans = PxTransFromQuatTrans(myTrans);

	auto* pShape = pPhysics_->createShape(physx::PxCapsuleGeometry(radius, halfHeight), *pMaterial_, true, DEFALT_SHAPE_FLAGS);

	physx::PxRigidDynamic* pActor = physx::PxCreateDynamic(*pPhysics_, trans, *pShape, density);

	pShape->release();

	setupDefaultRigidDynamic(*pActor);
	return reinterpret_cast<ActorHandle>(pActor);
}

ActorHandle XPhysics::createBox(const QuatTransf& myTrans, const AABB& bounds, float density)
{
	physx::PxTransform trans = PxTransFromQuatTrans(myTrans);
	physx::PxBoxGeometry geo(Px3FromVec3(bounds.halfVec()));

	auto* pShape = pPhysics_->createShape(geo, *pMaterial_, true, DEFALT_SHAPE_FLAGS);

	physx::PxRigidDynamic* pActor = physx::PxCreateDynamic(*pPhysics_, trans, *pShape, density);

	pShape->release();

	setupDefaultRigidDynamic(*pActor);
	return reinterpret_cast<ActorHandle>(pActor);
}

// ------------------------------------------

ActorHandle XPhysics::createStaticPlane(const QuatTransf& myTrans)
{
	physx::PxTransform trans = PxTransFromQuatTrans(myTrans);

	auto* pShape = pPhysics_->createShape(physx::PxPlaneGeometry(), *pMaterial_, true, DEFALT_SHAPE_FLAGS);

	// calls: createRigidStatic, attachShape
	physx::PxRigidStatic* pActor = physx::PxCreateStatic(*pPhysics_, trans, *pShape);

	pShape->release();

	setupDefaultRigidStatic(*pActor);
	return reinterpret_cast<ActorHandle>(pActor);
}

ActorHandle XPhysics::createStaticSphere(const QuatTransf& myTrans, float radius)
{
	physx::PxTransform trans = PxTransFromQuatTrans(myTrans);

	auto* pShape = pPhysics_->createShape(physx::PxSphereGeometry(radius), *pMaterial_, true, DEFALT_SHAPE_FLAGS);

	// calls: createRigidStatic, attachShape
	physx::PxRigidStatic* pActor = physx::PxCreateStatic(*pPhysics_, trans, *pShape);

	pShape->release();

	setupDefaultRigidStatic(*pActor);
	return reinterpret_cast<ActorHandle>(pActor);
}

ActorHandle XPhysics::createStaticCapsule(const QuatTransf& myTrans, float radius, float halfHeight)
{
	physx::PxTransform trans = PxTransFromQuatTrans(myTrans);

	auto* pShape = pPhysics_->createShape(physx::PxCapsuleGeometry(radius, halfHeight), *pMaterial_, true, DEFALT_SHAPE_FLAGS);

	// calls: createRigidStatic, attachShape
	physx::PxRigidStatic* pActor = physx::PxCreateStatic(*pPhysics_, trans, *pShape);

	pShape->release();

	setupDefaultRigidStatic(*pActor);
	return reinterpret_cast<ActorHandle>(pActor);
}

ActorHandle XPhysics::createStaticBox(const QuatTransf& myTrans, const AABB& bounds)
{
	physx::PxTransform trans = PxTransFromQuatTrans(myTrans);
	physx::PxBoxGeometry geo(Px3FromVec3(bounds.halfVec()));

	auto* pShape = pPhysics_->createShape(geo, *pMaterial_, true, DEFALT_SHAPE_FLAGS);

	// calls: createRigidStatic, attachShape
	physx::PxRigidStatic* pActor = physx::PxCreateStatic(*pPhysics_, trans, *pShape);

	pShape->release();

	setupDefaultRigidStatic(*pActor);
	return reinterpret_cast<ActorHandle>(pActor);
}

ActorHandle XPhysics::createStaticTrigger(const QuatTransf& myTrans, const AABB& bounds)
{
	physx::PxTransform trans = PxTransFromQuatTrans(myTrans);
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

	
void XPhysics::setupDefaultRigidDynamic(physx::PxRigidDynamic& body, bool kinematic)
{
	body.setActorFlag(physx::PxActorFlag::eVISUALIZATION, true);
	body.setAngularDamping(0.5f);
	body.setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, kinematic);
}


void XPhysics::setupDefaultRigidStatic(physx::PxRigidStatic& body)
{
	body.setActorFlag(physx::PxActorFlag::eVISUALIZATION, true);
	body.setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, true);
}



void XPhysics::onPvdSendClassDescriptions(PVD::PvdConnection& conn)
{
	X_UNUSED(conn);

}

void XPhysics::onPvdConnected(PVD::PvdConnection& conn)
{
	X_UNUSED(conn);

	//setup joint visualization.  This gets piped to pvd.
	pPhysics_->getVisualDebugger()->setVisualizeConstraints(true);
	pPhysics_->getVisualDebugger()->setVisualDebuggerFlag(physx::PxVisualDebuggerFlag::eTRANSMIT_CONTACTS, true);
	pPhysics_->getVisualDebugger()->setVisualDebuggerFlag(physx::PxVisualDebuggerFlag::eTRANSMIT_SCENEQUERIES, true);
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

	if (pObserved->is<physx::PxRigidActor>())
	{
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
	X_ERROR("Phys", "Obbject out of bounds. Name: \"%S\"", actor.getName());
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


void XPhysics::onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count)
{
	X_UNUSED(constraints);
	X_UNUSED(count);
}

void XPhysics::onWake(physx::PxActor** actors, physx::PxU32 count)
{
	X_UNUSED(actors);
	X_UNUSED(count);
	X_LOG0("Phys", "onWake: num: %" PRIu32, count);
}

void XPhysics::onSleep(physx::PxActor** actors, physx::PxU32 count)
{
	X_UNUSED(actors);
	X_UNUSED(count);

	X_LOG0("Phys", "onSleep: num: %" PRIu32, count);
}

void XPhysics::onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 count)
{
	X_UNUSED(pairHeader);
	X_UNUSED(pairs);
	X_UNUSED(count);

	X_LOG0("Phys", "onContact: num: %" PRIu32, count);
}

void XPhysics::onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count)
{
	X_UNUSED(pairs);
	X_UNUSED(count);

	X_LOG0("Phys", "onTrigger: num: %" PRIu32, count);

}


void XPhysics::togglePvdConnection(void)
{
	if (!pPhysics_->getPvdConnectionManager())
		return;

	if (pPhysics_->getPvdConnectionManager()->isConnected()) {
		pPhysics_->getPvdConnectionManager()->disconnect();
	}
	else {
		createPvdConnection();
	}
}

void XPhysics::createPvdConnection(void)
{
	physx::PxVisualDebuggerConnectionManager* pvd = pPhysics_->getPvdConnectionManager();
	if (!pvd) {
		X_ERROR("PhysicsSys", "Failed to get PVD connection manager");
		return;
	}

	//The connection flags state overall what data is to be sent to PVD.  Currently
	//the Debug connection flag requires support from the implementation (don't send
	//the data when debug isn't set) but the other two flags, profile and memory
	//are taken care of by the PVD SDK.

	//Use these flags for a clean profile trace with minimal overhead
	physx::PxVisualDebuggerConnectionFlags theConnectionFlags(
		physx::PxVisualDebuggerConnectionFlag::eDEBUG |
		physx::PxVisualDebuggerConnectionFlag::ePROFILE |
		physx::PxVisualDebuggerConnectionFlag::eMEMORY);

	if (!pvdParams_.useFullPvdConnection) {
		theConnectionFlags = physx::PxVisualDebuggerConnectionFlag::ePROFILE;
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

	physx::PxVisualDebuggerConnection* pCon = physx::PxVisualDebuggerExt::createConnection(pvd, pvdParams_.ip.c_str(), pvdParams_.port, pvdParams_.timeout, theConnectionFlags);
	if (pCon && pCon->isConnected()) {
		X_LOG1("PhysicsSys", "Connected to PVD");
	}
}


// --------------------------------------------------------




Stepper* XPhysics::getStepper(void)
{
	switch (stepperType_)
	{
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
		X_ERROR("PhysicsSys", "Scratch block must be a multiple of %i. requested size: %i", MULTIPLE_OF, size);
		return;
	}

	if (pScratchBlock_) {
		X_DELETE_ARRAY(pScratchBlock_, g_PhysicsArena);
	}

	pScratchBlock_ = X_NEW_ARRAY_ALIGNED(uint8_t, size, g_PhysicsArena, "ScratchBlock", 16);
	scratchBlockSize_ = size;
}


void XPhysics::onDebugDrawChange(bool enabled)
{
	if (enabled && !pDebugRender_) {
		if (!initDebugRenderer())
		{
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