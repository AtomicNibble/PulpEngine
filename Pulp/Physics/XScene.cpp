#include "stdafx.h"
#include "XScene.h"

#include "Util/MathHelpers.h"
#include "Vars/PhysicsVars.h"
#include "DebugRender/DebugRender.h"
#include "ControllerWrapper.h"
#include "BatchQuery.h"

#include <characterkinematic\PxControllerManager.h>

X_NAMESPACE_BEGIN(physics)

namespace
{
	// some asserts to check we can just cast directly to px types.

	static_assert(HitFlag::POSITION == physx::PxHitFlag::ePOSITION, "HitFlag mismatch");
	static_assert(HitFlag::NORMAL == physx::PxHitFlag::eNORMAL, "HitFlag mismatch");
	static_assert(HitFlag::DISTANCE == physx::PxHitFlag::eDISTANCE, "HitFlag mismatch");
	static_assert(HitFlag::UV == physx::PxHitFlag::eUV, "HitFlag mismatch");
	static_assert(HitFlag::ASSUME_NO_INITIAL_OVERLAP == physx::PxHitFlag::eASSUME_NO_INITIAL_OVERLAP, "HitFlag mismatch");
	static_assert(HitFlag::MESH_MULTIPLE == physx::PxHitFlag::eMESH_MULTIPLE, "HitFlag mismatch");
	static_assert(HitFlag::MESH_ANY == physx::PxHitFlag::eMESH_ANY, "HitFlag mismatch");
	static_assert(HitFlag::MESH_BOTH_SIDES == physx::PxHitFlag::eMESH_BOTH_SIDES, "HitFlag mismatch");
	static_assert(HitFlag::PRECISE_SWEEP == physx::PxHitFlag::ePRECISE_SWEEP, "HitFlag mismatch");
	static_assert(HitFlag::MTD == physx::PxHitFlag::eMTD, "HitFlag mismatch");


	static_assert(QueryFlag::STATIC == physx::PxQueryFlag::eSTATIC, "QueryFlag mismatch");
	static_assert(QueryFlag::DYNAMIC == physx::PxQueryFlag::eDYNAMIC, "QueryFlag mismatch");
	static_assert(QueryFlag::PREFILTER == physx::PxQueryFlag::ePREFILTER, "QueryFlag mismatch");
	static_assert(QueryFlag::POSTFILTER == physx::PxQueryFlag::ePOSTFILTER, "QueryFlag mismatch");
	static_assert(QueryFlag::ANY_HIT == physx::PxQueryFlag::eANY_HIT, "QueryFlag mismatch");
	static_assert(QueryFlag::NO_BLOCK == physx::PxQueryFlag::eNO_BLOCK, "QueryFlag mismatch");


	X_ENSURE_SIZE(ActorShape, sizeof(physx::PxActorShape));
	X_ENSURE_SIZE(QueryHit, sizeof(physx::PxQueryHit));
	X_ENSURE_SIZE(LocationHit, sizeof(physx::PxLocationHit));
	X_ENSURE_SIZE(RaycastHit, sizeof(physx::PxRaycastHit));
	X_ENSURE_SIZE(OverlapHit, sizeof(physx::PxOverlapHit));
	X_ENSURE_SIZE(SweepHit, sizeof(physx::PxSweepHit));


	X_ENSURE_SIZE(HitCallback<ActorShape>, sizeof(physx::PxHitCallback<physx::PxActorShape>));
	X_ENSURE_SIZE(HitCallback<OverlapHit>, sizeof(physx::PxHitCallback<physx::PxOverlapHit>));
	X_ENSURE_SIZE(HitCallback<SweepHit>, sizeof(physx::PxHitCallback<physx::PxSweepHit>));

	X_ENSURE_SIZE(HitBuffer<RaycastHit>, sizeof(physx::PxHitBuffer<physx::PxRaycastHit>));
	X_ENSURE_SIZE(HitBuffer<OverlapHit>, sizeof(physx::PxHitBuffer<physx::PxOverlapHit>));
	X_ENSURE_SIZE(HitBuffer<SweepHit>, sizeof(physx::PxHitBuffer<physx::PxSweepHit>));

	X_ENSURE_SIZE(RaycastQueryResult, sizeof(physx::PxRaycastQueryResult));
	X_ENSURE_SIZE(SweepQueryResult, sizeof(physx::PxSweepQueryResult));
	X_ENSURE_SIZE(OverlapQueryResult, sizeof(physx::PxOverlapQueryResult));


	X_ENSURE_SIZE(ActiveTransform, sizeof(physx::PxActiveTransform));

	static_assert(X_OFFSETOF(ActorShape, actor) == X_OFFSETOF(physx::PxActorShape, actor), "Offset don't match");
	static_assert(X_OFFSETOF(ActorShape, pShape) == X_OFFSETOF(physx::PxActorShape, shape), "Offset don't match");

	static_assert(X_OFFSETOF(QueryHit, faceIndex) == X_OFFSETOF(physx::PxQueryHit, faceIndex), "Offset don't match");

	static_assert(X_OFFSETOF(LocationHit, flags) == X_OFFSETOF(physx::PxLocationHit, flags), "Offset don't match");
	static_assert(X_OFFSETOF(LocationHit, position) == X_OFFSETOF(physx::PxLocationHit, position), "Offset don't match");
	static_assert(X_OFFSETOF(LocationHit, normal) == X_OFFSETOF(physx::PxLocationHit, normal), "Offset don't match");
	static_assert(X_OFFSETOF(LocationHit, distance) == X_OFFSETOF(physx::PxLocationHit, distance), "Offset don't match");


	static_assert(X_OFFSETOF(RaycastHit, u) == X_OFFSETOF(physx::PxRaycastHit, u), "Offset don't match");
	static_assert(X_OFFSETOF(RaycastHit, v) == X_OFFSETOF(physx::PxRaycastHit, v), "Offset don't match");

	static_assert(X_OFFSETOF(OverlapHit, padTo16Bytes) == X_OFFSETOF(physx::PxOverlapHit, padTo16Bytes), "Offset don't match");

	static_assert(X_OFFSETOF(SweepHit, padTo16Bytes) == X_OFFSETOF(physx::PxSweepHit, padTo16Bytes), "Offset don't match");

	static_assert(X_OFFSETOF(RaycastCallback, block) == X_OFFSETOF(physx::PxRaycastCallback, block), "Offset don't match");
	static_assert(X_OFFSETOF(RaycastCallback, hasBlock) == X_OFFSETOF(physx::PxRaycastCallback, hasBlock), "Offset don't match");
	static_assert(X_OFFSETOF(RaycastCallback, pTouches) == X_OFFSETOF(physx::PxRaycastCallback, touches), "Offset don't match");
	static_assert(X_OFFSETOF(RaycastCallback, maxNbTouches) == X_OFFSETOF(physx::PxRaycastCallback, maxNbTouches), "Offset don't match");
	static_assert(X_OFFSETOF(RaycastCallback, nbTouches) == X_OFFSETOF(physx::PxRaycastCallback, nbTouches), "Offset don't match");


	static_assert(X_OFFSETOF(RaycastQueryResult, block) == X_OFFSETOF(physx::PxRaycastQueryResult, block), "Offset don't match");
	static_assert(X_OFFSETOF(RaycastQueryResult, pTouches) == X_OFFSETOF(physx::PxRaycastQueryResult, touches), "Offset don't match");
	static_assert(X_OFFSETOF(RaycastQueryResult, nbTouches) == X_OFFSETOF(physx::PxRaycastQueryResult, nbTouches), "Offset don't match");
	static_assert(X_OFFSETOF(RaycastQueryResult, pUserData) == X_OFFSETOF(physx::PxRaycastQueryResult, userData), "Offset don't match");
	static_assert(X_OFFSETOF(RaycastQueryResult, queryStatus) == X_OFFSETOF(physx::PxRaycastQueryResult, queryStatus), "Offset don't match");
	static_assert(X_OFFSETOF(RaycastQueryResult, hasBlock) == X_OFFSETOF(physx::PxRaycastQueryResult, hasBlock), "Offset don't match");
	static_assert(X_OFFSETOF(RaycastQueryResult, pad) == X_OFFSETOF(physx::PxRaycastQueryResult, pad), "Offset don't match");

	static_assert(X_OFFSETOF(ActiveTransform, actor) == X_OFFSETOF(physx::PxActiveTransform, actor), "Offset don't match");
	static_assert(X_OFFSETOF(ActiveTransform, userData) == X_OFFSETOF(physx::PxActiveTransform, userData), "Offset don't match");
	static_assert(X_OFFSETOF(ActiveTransform, actor2World) == X_OFFSETOF(physx::PxActiveTransform, actor2World), "Offset don't match");


	void copycontrollerDesc(physx::PxControllerDesc& pxDesc, const ControllerDesc& desc)
	{
		pxDesc.position = Px3ExtFromVec3(desc.position);
		pxDesc.upDirection = Px3FromVec3(desc.upDirection);
		pxDesc.slopeLimit = desc.slopeLimit;
		pxDesc.invisibleWallHeight = desc.invisibleWallHeight;
		pxDesc.maxJumpHeight = desc.maxJumpHeight;
		pxDesc.contactOffset = desc.contactOffset;
		pxDesc.stepOffset = desc.stepOffset;
		pxDesc.density = desc.density;
		pxDesc.scaleCoeff = desc.scaleCoeff;
		pxDesc.volumeGrowth = desc.volumeGrowth;
		pxDesc.material = reinterpret_cast<physx::PxMaterial*>(desc.material);
		if (desc.nonWalkableMode == ControllerDesc::NonWalkableMode::PreventClimbing) {
			pxDesc.nonWalkableMode = physx::PxControllerNonWalkableMode::ePREVENT_CLIMBING;
		}
		else if (desc.nonWalkableMode == ControllerDesc::NonWalkableMode::PreventClimbingAndForceSliding) {
			pxDesc.nonWalkableMode = physx::PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING;
		}
		else {
			X_ASSERT_UNREACHABLE();
		}
	}

} // namespace

XScene::XScene(PhysXVars& vars, physx::PxPhysics* pPhysics, core::MemoryArenaBase* arena) :
	vars_(vars),
	arena_(X_ASSERT_NOT_NULL(arena)),
	pPhysics_(X_ASSERT_NOT_NULL(pPhysics)),
	pScene_(nullptr),
	pControllerManager_(nullptr),
	triggerPairs_(arena)
{

}

XScene::~XScene()
{
	if (pScene_) {
		pScene_->fetchResults(true);
	}

	// we need a read, maybe write lock here.
	if(pScene_)
	{
		PHYS_SCENE_WRITE_LOCK(pScene_);
		core::SafeRelease(pControllerManager_);
	}

	core::SafeRelease(pScene_);
}

bool XScene::createPxScene(physx::PxSceneDesc& pxDesc)
{
	X_ASSERT(pScene_ == nullptr, "Scenen alread created")();
	X_ASSERT(pControllerManager_ == nullptr, "Scenen alread created")();
	X_ASSERT(pxDesc.simulationEventCallback == nullptr, "SimEventCallback must be null")();

	if (!pxDesc.isValid()) {
		X_ERROR("PhysScene", "Scene description is invalid");
		return false;
	}

	pxDesc.simulationEventCallback = this;

	pScene_ = pPhysics_->createScene(pxDesc);
	if (!pScene_) {
		X_ERROR("PhysScene", "Failed to create scene");
		return false;
	}


#if PHYSX_SCENE_REQUIRES_LOCK
	const bool lock = false; // this don't result in scene lock so we get api errors :/
#else
	const bool lock = false;
#endif // !PHYSX_SCENE_REQUIRES_LOCK


	pControllerManager_ = PxCreateControllerManager(*pScene_, lock);
	if (!pControllerManager_) {
		X_ERROR("PhysScene", "Failed to create controller manager");
		return false;
	}

	return true;
}

// ------------------------------------------

void XScene::drawDebug(DebugRender* pDebugRender) const
{
	const physx::PxRenderBuffer& debugRenderable = pScene_->getRenderBuffer();

	pDebugRender->update(debugRenderable);
}

void XScene::setVisualizationCullingBox(const AABB& box)
{
	physx::PxBounds3 bounds(Px3FromVec3(box.min), Px3FromVec3(box.max));

	PHYS_SCENE_WRITE_LOCK(pScene_);
	pScene_->setVisualizationCullingBox(bounds);
}

// ------------------------------------------

LockHandle XScene::lock(bool write)
{
#if PHYSX_SCENE_REQUIRES_LOCK

	if (write)
	{
		pScene_->lockWrite(__FILE__, __LINE__);
	}
	else
	{
		pScene_->lockRead(__FILE__, __LINE__);
	}

#else
	X_UNUSED(write);
#endif // !PHYSX_SCENE_REQUIRES_LOCK

	return static_cast<LockHandle>(write);
}

void XScene::unLock(LockHandle lock)
{
#if PHYSX_SCENE_REQUIRES_LOCK
	const bool writeLock = lock != 0;

	if (writeLock)
	{
		pScene_->unlockWrite();
	}
	else
	{
		pScene_->unlockRead();
	}
#else
	X_UNUSED(lock);
#endif // !PHYSX_SCENE_REQUIRES_LOCK
}

// ------------------------------------------

void XScene::setGravity(const Vec3f& gravity)
{
	vars_.SetGravityVecValue(gravity);

	PHYS_SCENE_WRITE_LOCK(pScene_);
	pScene_->setGravity(Px3FromVec3(gravity));
}

void XScene::setBounceThresholdVelocity(float32_t bounceThresholdVelocity)
{
	PHYS_SCENE_WRITE_LOCK(pScene_);
	pScene_->setBounceThresholdVelocity(bounceThresholdVelocity);
}

// ------------------------------------------


RegionHandle XScene::addRegion(const AABB& bounds)
{
	PHYS_SCENE_WRITE_LOCK(pScene_);

	if (pScene_->getBroadPhaseType() != physx::PxBroadPhaseType::eMBP) {
		return 0;
	}

	physx::PxBroadPhaseRegion region;
	region.bounds = PxBounds3FromAABB(bounds);
	region.userData = nullptr;

	uint32_t handle = pScene_->addBroadPhaseRegion(region);

	if (handle == 0xFFFFFFFF) {
		core::StackString256 tmp;
		X_ERROR("Phys", "error adding region with bounds: %s", bounds.toString(tmp));
	}

	return handle;
}

bool XScene::removeRegion(RegionHandle handle_)
{
	PHYS_SCENE_WRITE_LOCK(pScene_);

	if (pScene_->getBroadPhaseType() != physx::PxBroadPhaseType::eMBP) {
		return true;
	}

	uint32_t handle = safe_static_cast<uint32_t>(handle_);

	if (!pScene_->removeBroadPhaseRegion(handle)) {
		X_ERROR("PhysScene", "Failed to remove region id: %" PRIu32, handle);
		return false;
	}

	return true;
}

// ------------------------------------------


void XScene::addActorToScene(ActorHandle handle)
{
	physx::PxRigidActor& actor = *reinterpret_cast<physx::PxRigidActor*>(handle);

	PHYS_SCENE_WRITE_LOCK(pScene_);

	pScene_->addActor(actor);
}

void XScene::addActorToScene(ActorHandle handle, const char* pDebugNamePointer)
{
	physx::PxRigidActor& actor = *reinterpret_cast<physx::PxRigidActor*>(handle);
	actor.setName(pDebugNamePointer);

	PHYS_SCENE_WRITE_LOCK(pScene_);

	pScene_->addActor(actor);
}

void XScene::addActorsToScene(ActorHandle* pHandles, size_t num)
{
	physx::PxActor* const pActors = reinterpret_cast<physx::PxActor* const>(pHandles);

	PHYS_SCENE_WRITE_LOCK(pScene_);

	pScene_->addActors(&pActors, safe_static_cast<physx::PxU32>(num));
}

void XScene::removeActor(ActorHandle handle)
{
	physx::PxRigidActor& actor = *reinterpret_cast<physx::PxRigidActor*>(handle);

	PHYS_SCENE_WRITE_LOCK(pScene_);

	pScene_->removeActor(actor, true);
}

void XScene::removeActors(ActorHandle* pHandles, size_t num)
{
	physx::PxActor* const pActors = reinterpret_cast<physx::PxActor* const>(pHandles);

	PHYS_SCENE_WRITE_LOCK(pScene_);

	pScene_->removeActors(&pActors, safe_static_cast<physx::PxU32>(num), true);
}

// ------------------------------------------

void XScene::addAggregate(AggregateHandle handle)
{
	physx::PxAggregate& agg = *reinterpret_cast<physx::PxAggregate*>(handle);

	PHYS_SCENE_WRITE_LOCK(pScene_);

	pScene_->addAggregate(agg);
}

void XScene::removeAggregate(AggregateHandle handle)
{
	physx::PxAggregate& agg = *reinterpret_cast<physx::PxAggregate*>(handle);

	PHYS_SCENE_WRITE_LOCK(pScene_);

	pScene_->removeAggregate(agg, true);
}

// ------------------------------------------

ICharacterController* XScene::createCharacterController(const ControllerDesc& desc)
{
	if (desc.material == INVALID_HANLDE) {
		X_ERROR("Phys", "Controller missing material");
		return nullptr;
	}

	if (desc.shape == ControllerDesc::ShapeType::Box)
	{
		physx::PxBoxControllerDesc pxDesc;
		copycontrollerDesc(pxDesc, desc);

		const BoxControllerDesc& boxDesc = static_cast<const BoxControllerDesc&>(desc);
		pxDesc.halfHeight = boxDesc.halfHeight;
		pxDesc.halfSideExtent = boxDesc.halfSideExtent;
		pxDesc.halfForwardExtent = boxDesc.halfForwardExtent;
		pxDesc.reportCallback = this;

		if (!pxDesc.isValid()) {
			X_ERROR("Phys", "Box controller desc is invalid");
			return nullptr;
		}

		physx::PxController* pController = nullptr;
		{
			PHYS_SCENE_WRITE_LOCK(pScene_);

			pController = pControllerManager_->createController(pxDesc);
		}

		X_ASSERT_NOT_NULL(pController);

		return X_NEW(XBoxCharController, arena_, "BoxCharController")(static_cast<physx::PxBoxController*>(pController));
	}
	if (desc.shape == ControllerDesc::ShapeType::Capsule)
	{
		physx::PxCapsuleControllerDesc pxDesc;
		copycontrollerDesc(pxDesc, desc);

		const CapsuleControllerDesc& capsulDesc = static_cast<const CapsuleControllerDesc&>(desc);
		pxDesc.radius = capsulDesc.radius;
		pxDesc.height = capsulDesc.height;
		pxDesc.reportCallback = this;
		if (capsulDesc.climbingMode == CapsuleControllerDesc::ClimbingMode::Easy) {
			pxDesc.climbingMode = physx::PxCapsuleClimbingMode::eEASY;
		}
		else if (capsulDesc.climbingMode == CapsuleControllerDesc::ClimbingMode::Constrained) {
			pxDesc.climbingMode = physx::PxCapsuleClimbingMode::eCONSTRAINED;
		}
		else {
			X_ASSERT_UNREACHABLE();
		}

		if (!pxDesc.isValid()) {
			X_ERROR("Phys", "Capsule controller desc is invalid");
 			return nullptr;
		}

		physx::PxController* pController = nullptr;
		{
			PHYS_SCENE_WRITE_LOCK(pScene_);

			pController = pControllerManager_->createController(pxDesc);
		}

		X_ASSERT_NOT_NULL(pController);

		return X_NEW(XCapsuleCharController, arena_, "CapsuleCharController")(static_cast<physx::PxCapsuleController*>(pController));
	}

	X_ASSERT_UNREACHABLE();
	return nullptr;
}

void XScene::releaseCharacterController(ICharacterController* pController)
{
	PHYS_SCENE_WRITE_LOCK(pScene_);

	X_DELETE(pController, arena_);
}

// set transforms
void XScene::setKinematicTarget(ActorHandle handle, const Transformf& destination)
{
	physx::PxTransform trans = PxTransFromQuatTrans(destination);
	physx::PxRigidActor* pActor = reinterpret_cast<physx::PxRigidActor*>(handle);
	if (!pActor->is<physx::PxRigidDynamic>())
	{
		X_ERROR("Phys", "Setting kinematic target on a none dynamic actor is not allowed");
		return;
	}

	PHYS_SCENE_WRITE_LOCK(pScene_);

	physx::PxRigidDynamic* pDynActor = static_cast<physx::PxRigidDynamic*>(pActor);
	auto flags = pDynActor->getRigidBodyFlags();

	if (!flags.isSet(physx::PxRigidBodyFlag::eKINEMATIC))
	{
		X_WARNING("Phys", "Setting kinematic target on a none kinematic actor");
	}

	pDynActor->setKinematicTarget(trans);
}

void XScene::setGlobalPose(ActorHandle handle, const Transformf& destination)
{
	physx::PxTransform trans = PxTransFromQuatTrans(destination);
	physx::PxRigidActor* pActor = reinterpret_cast<physx::PxRigidActor*>(handle);

	PHYS_SCENE_WRITE_LOCK(pScene_);

	pActor->setGlobalPose(trans);
}


// ------------------------------------------

bool XScene::raycast(const Vec3f& origin, const Vec3f& unitDir, const float32_t distance,
	RaycastCallback& hitCall, HitFlags hitFlags, QueryFlags queryFlags) const
{
	PHYS_SCENE_READ_LOCK(pScene_);

	// static assets check this stuff maps.
	return pScene_->raycast(
		Px3FromVec3(origin), 
		Px3FromVec3(unitDir), 
		distance,
		reinterpret_cast<physx::PxRaycastCallback&>(hitCall), 
		static_cast<physx::PxHitFlags>(hitFlags.ToInt()),
		physx::PxQueryFilterData(static_cast<physx::PxQueryFlags>(queryFlags.ToInt()))
	);
}

bool XScene::sweep(const GeometryBase& geometry, const Transformf& pose, const Vec3f& unitDir, const float32_t distance,
	SweepCallback& hitCall, HitFlags hitFlags, QueryFlags queryFlags, const float32_t inflation) const
{
	PHYS_SCENE_READ_LOCK(pScene_);

	return pScene_->sweep(
		PxGeoFromGeo(geometry),
		PxTransFromQuatTrans(pose),
		Px3FromVec3(unitDir),
		distance,
		reinterpret_cast<physx::PxSweepCallback&>(hitCall),
		static_cast<physx::PxHitFlags>(hitFlags.ToInt()),
		physx::PxQueryFilterData(static_cast<physx::PxQueryFlags>(queryFlags.ToInt()))
	);
}

bool XScene::overlap(const GeometryBase& geometry, const Transformf& pose, OverlapCallback& hitCall) const
{
	PHYS_SCENE_READ_LOCK(pScene_);

	return pScene_->overlap(
		PxGeoFromGeo(geometry),
		PxTransFromQuatTrans(pose),
		reinterpret_cast<physx::PxOverlapCallback&>(hitCall)
	);
}

IBatchedQuery* XScene::createBatchQuery(const QueryMemory& desc)
{
	physx::PxBatchQueryDesc pxDesc(
		static_cast<physx::PxU32>(desc.raycastTouchBufferSize),
		static_cast<physx::PxU32>(desc.sweepTouchBufferSize),
		static_cast<physx::PxU32>(desc.overlapTouchBufferSize)
	);

	pxDesc.queryMemory.userRaycastResultBuffer	= reinterpret_cast<physx::PxRaycastQueryResult*>(desc.pUserRaycastResultBuffer);
	pxDesc.queryMemory.userRaycastTouchBuffer	= reinterpret_cast<physx::PxRaycastHit*>(desc.pUserRaycastTouchBuffer);
	pxDesc.queryMemory.userSweepResultBuffer	= reinterpret_cast<physx::PxSweepQueryResult*>(desc.pUserSweepResultBuffer);
	pxDesc.queryMemory.userSweepTouchBuffer		= reinterpret_cast<physx::PxSweepHit*>(desc.pUserSweepTouchBuffer);
	pxDesc.queryMemory.userOverlapResultBuffer	= reinterpret_cast<physx::PxOverlapQueryResult*>(desc.pUserOverlapResultBuffer);
	pxDesc.queryMemory.userOverlapTouchBuffer	= reinterpret_cast<physx::PxOverlapHit*>(desc.pUserOverlapTouchBuffer);

	if (!pxDesc.isValid()) {
		X_ERROR("Physics", "Batched query description is invalid");
		return nullptr;
	}

	// props need write lock not sure.
	// since it's not editing the scene objects :/
	physx::PxBatchQuery* pBatched = nullptr;
	{
		PHYS_SCENE_WRITE_LOCK(pScene_);

		pBatched = pScene_->createBatchQuery(pxDesc);
		if (!pBatched) {
			return nullptr;
		}
	}

	return X_NEW(BatchedQuery, arena_, "BatchedQuery")(pBatched, arena_);
}

// ------------------------------------------

const ActiveTransform* XScene::getActiveTransforms(size_t& numTransformsOut)
{
	PHYS_SCENE_READ_LOCK(pScene_);

	physx::PxU32 num = 0;
	const physx::PxActiveTransform* pTrans = pScene_->getActiveTransforms(num);

	numTransformsOut = num;
	return reinterpret_cast<const ActiveTransform*>(pTrans);
}

const TriggerPair* XScene::getTriggerPairs(size_t& numTriggerPairs)
{
	numTriggerPairs = triggerPairs_.size();
	return triggerPairs_.data();
}


// ------------------------------------------

void XScene::onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count)
{
	X_UNUSED(constraints);
	X_UNUSED(count);
}

void XScene::onWake(physx::PxActor** actors, physx::PxU32 count)
{
	X_UNUSED(actors);
	X_UNUSED(count);
	X_LOG0("Phys", "onWake: num: %" PRIu32, count);
}

void XScene::onSleep(physx::PxActor** actors, physx::PxU32 count)
{
	X_UNUSED(actors);
	X_UNUSED(count);

	X_LOG0("Phys", "onSleep: num: %" PRIu32, count);
}

void XScene::onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 count)
{
	X_UNUSED(pairHeader);
	X_UNUSED(pairs);
	X_UNUSED(count);

	X_LOG0("Phys", "onContact: num: %" PRIu32, count);
}

void XScene::onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count)
{
	X_UNUSED(pairs);
	X_UNUSED(count);

	X_LOG0("Phys", "onTrigger: num: %" PRIu32, count);

	for (uint32_t i = 0; i < count; i++)
	{
		auto& pair = pairs[i];
		// ignore pairs when shapes have been deleted
		if (pair.flags & (physx::PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER | physx::PxTriggerPairFlag::eREMOVED_SHAPE_OTHER)) {
			continue;
		}

	
		triggerPairs_.emplace_back(reinterpret_cast<ActorHandle>(pair.triggerActor), reinterpret_cast<ActorHandle>(pair.otherActor));
	}
}

// ------------------------------------------

void XScene::onShapeHit(const physx::PxControllerShapeHit& hit)
{
	X_UNUSED(hit);

	physx::PxRigidDynamic* actor = hit.shape->getActor()->is<physx::PxRigidDynamic>();
	if (actor)
	{
		if (actor->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC) {
			return;
		}

		// We only allow horizontal pushes. Vertical pushes when we stand on dynamic objects creates
		// useless stress on the solver. It would be possible to enable/disable vertical pushes on
		// particular objects, if the gameplay requires it.
		const physx::PxVec3 upVector = hit.controller->getUpDirection();
		const physx::PxF32 dp = hit.dir.dot(upVector);
		if (fabsf(dp)<1e-3f)
		{
			physx::PxReal coeff = actor->getMass() * hit.length;
			physx::PxRigidBodyExt::addForceAtLocalPos(
				*actor, 
				hit.dir*coeff,
				physx::PxVec3(0, 0, 0),
				physx::PxForceMode::eIMPULSE
			);
		}
	}
	 
}

void XScene::onControllerHit(const physx::PxControllersHit& hit)
{
	X_UNUSED(hit);

}

void XScene::onObstacleHit(const physx::PxControllerObstacleHit& hit)
{
	X_UNUSED(hit);


}

X_NAMESPACE_END