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

	// Characters controllers
	ICharacterController* createCharacterController(const ControllerDesc& desc) X_FINAL;
	void releaseCharacterController(ICharacterController* pController) X_FINAL;

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