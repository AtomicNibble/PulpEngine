#include "stdafx.h"
#include "XScene.h"

#include "MathHelpers.h"
#include "PhysicsVars.h"
#include "DebugRender.h"
#include "ControllerWrapper.h"

#include <characterkinematic\PxControllerManager.h>

X_NAMESPACE_BEGIN(physics)

namespace
{

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
	arena_(arena),
	pPhysics_(pPhysics),
	pScene_(nullptr),
	pControllerManager_(nullptr)
{

}

XScene::~XScene()
{
	if (pScene_) {
		pScene_->fetchResults(true);
	}

	core::SafeRelease(pControllerManager_);
	core::SafeRelease(pScene_);
}

bool XScene::createPxScene(const physx::PxSceneDesc& pxDesc)
{
	X_ASSERT(pScene_ == nullptr, "Scenen alread created")();
	X_ASSERT(pControllerManager_ == nullptr, "Scenen alread created")();

	if (!pxDesc.isValid()) {
		X_ERROR("PhysScene", "Scene description is invalid");
		return false;
	}

	pScene_ = pPhysics_->createScene(pxDesc);
	if (!pScene_) {
		X_ERROR("PhysScene", "Failed to create scene");
		return false;
	}

	pControllerManager_ = PxCreateControllerManager(*pScene_);
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
	physx::PxBounds3 bounds;
	bounds.minimum = Px3FromVec3(box.min);
	bounds.maximum = Px3FromVec3(box.max);

	physx::PxSceneWriteLock scopedLock(*pScene_);
	pScene_->setVisualizationCullingBox(bounds);
}

// ------------------------------------------

void XScene::setGravity(const Vec3f& gravity)
{
	vars_.SetGravityVecValue(gravity);

	physx::PxSceneWriteLock scopedLock(*pScene_);

	pScene_->setGravity(Px3FromVec3(gravity));
}

void XScene::setBounceThresholdVelocity(float32_t bounceThresholdVelocity)
{
	physx::PxSceneWriteLock scopedLock(*pScene_);

	pScene_->setBounceThresholdVelocity(bounceThresholdVelocity);
}

// ------------------------------------------


RegionHandle XScene::addRegion(const AABB& bounds)
{
	physx::PxSceneWriteLock scopedLock(*pScene_);

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
	physx::PxSceneWriteLock scopedLock(*pScene_);

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

	physx::PxSceneWriteLock scopedLock(*pScene_);

	pScene_->addActor(actor);
}

void XScene::addActorToScene(ActorHandle handle, const char* pDebugNamePointer)
{
	physx::PxRigidActor& actor = *reinterpret_cast<physx::PxRigidActor*>(handle);
	actor.setName(pDebugNamePointer);

	physx::PxSceneWriteLock scopedLock(*pScene_);

	pScene_->addActor(actor);
}

void XScene::addActorsToScene(ActorHandle* pHandles, size_t num)
{
	physx::PxActor* pActors = reinterpret_cast<physx::PxActor*>(pHandles);

	physx::PxSceneWriteLock scopedLock(*pScene_);

	pScene_->addActors(&pActors, safe_static_cast<physx::PxU32>(num));
}

// ------------------------------------------

ICharacterController* XScene::createCharacterController(const ControllerDesc& desc)
{
	if (desc.shape == ControllerDesc::ShapeType::Box)
	{
		physx::PxBoxControllerDesc pxDesc;
		copycontrollerDesc(pxDesc, desc);

		const BoxControllerDesc& boxDesc = static_cast<const BoxControllerDesc&>(desc);
		pxDesc.halfHeight = boxDesc.halfHeight;
		pxDesc.halfSideExtent = boxDesc.halfSideExtent;
		pxDesc.halfForwardExtent = boxDesc.halfForwardExtent;

		auto* pController = pControllerManager_->createController(pxDesc);

		return X_NEW(XBoxCharController, arena_, "BoxCharController")(static_cast<physx::PxBoxController*>(pController));
	}
	if (desc.shape == ControllerDesc::ShapeType::Capsule)
	{
		physx::PxCapsuleControllerDesc pxDesc;
		copycontrollerDesc(pxDesc, desc);

		const CapsuleControllerDesc& boxDesc = static_cast<const CapsuleControllerDesc&>(desc);
		pxDesc.radius = boxDesc.radius;
		pxDesc.height = boxDesc.height;
		if (boxDesc.climbingMode == CapsuleControllerDesc::ClimbingMode::Easy) {
			pxDesc.climbingMode = physx::PxCapsuleClimbingMode::eEASY;
		}
		else if (boxDesc.climbingMode == CapsuleControllerDesc::ClimbingMode::Constrained) {
			pxDesc.climbingMode = physx::PxCapsuleClimbingMode::eCONSTRAINED;
		}
		else {
			X_ASSERT_UNREACHABLE();
		}

		auto* pController = pControllerManager_->createController(pxDesc);

		return X_NEW(XCapsuleCharController, arena_, "CapsuleCharController")(static_cast<physx::PxCapsuleController*>(pController));
	}

	X_ASSERT_UNREACHABLE();
	return nullptr;
}

void XScene::releaseCharacterController(ICharacterController* pController)
{
	X_DELETE(pController, arena_);
}

X_NAMESPACE_END