#pragma once

#include <IPhysics.h>

#include "extensions/PxExtensionsAPI.h"
#include "physxvisualdebuggersdk/PvdConnectionManager.h"

#include "Allocator.h"
#include "Logger.h"
#include "CpuDispatcher.h"
#include "Stepper.h"
#include "PhysicsVars.h"
#include "DebugRender.h"

namespace PVD {
	using namespace physx::debugger;
	using namespace physx::debugger::comm;
}

X_NAMESPACE_DECLARE(core,
	struct IConsoleCmdArgs;
)


X_NAMESPACE_BEGIN(physics)

class PhysCooking;
class XScene;

class XPhysics : 
	public IPhysics, 
	public PVD::PvdConnectionHandler, //receive notifications when pvd is connected and disconnected.
	public physx::PxDeletionListener,
	public physx::PxBroadPhaseCallback,
	public physx::PxSimulationEventCallback,
	public IStepperHandler
{

	struct PvdParameters
	{
		PvdParameters();

		core::StackString<128>	ip;
		uint32_t	port;
		uint32_t	timeout;
		bool	useFullPvdConnection;
	};

	X_NO_COPY(XPhysics);
	X_NO_ASSIGN(XPhysics);

public:
	static const size_t SCRATCH_BLOCK_SIZE = 1024 * 16;

	static const physx::PxShapeFlags DEFALT_SHAPE_FLAGS;

public:
	XPhysics(uint32_t maxSubSteps, core::V2::JobSystem* pJobSys, core::MemoryArenaBase* arena);
	~XPhysics() X_OVERRIDE;

	// IPhysics
	void registerVars(void) X_FINAL;
	void registerCmds(void) X_FINAL;

	bool init(const ToleranceScale& scale) X_FINAL;
	void shutDown(void) X_FINAL;
	void release(void) X_FINAL;

	void onTickPreRender(float dtime, const AABB& debugVisCullBounds) X_FINAL;
	void onTickPostRender(float dtime) X_FINAL;
	void render(void) X_FINAL;

	IPhysicsCooking* getCooking(void) X_FINAL;

	// Scene stuff
	IScene* createScene(const SceneDesc& desc) X_FINAL;
	void releaseScene(IScene* pScene) X_FINAL;

	// materials
	MaterialHandle createMaterial(MaterialDesc& desc) X_FINAL;

	// aggregates's
	AggregateHandle createAggregate(uint32_t maxActors, bool selfCollisions) X_FINAL;
	bool addActorToAggregate(AggregateHandle handle, ActorHandle actor) X_FINAL;
	bool releaseAggregate(AggregateHandle handle) X_FINAL;

	// joints
	IJoint* createJoint(JointType::Enum type, ActorHandle actor0, ActorHandle actor1,
		const QuatTransf& localFrame0, const QuatTransf& localFrame1) X_FINAL;
	void releaseJoint(IJoint* pJoint) X_FINAL;

	void setActorDebugNamePointer(ActorHandle handle, const char* pNamePointer) X_FINAL;
	void setActorDominanceGroup(ActorHandle handle, int8_t group) X_FINAL;
	void setGroupFlags(ActorHandle handle, const GroupFlags groupFlags) X_FINAL;

	// group collision
	bool GetGroupCollisionFlag(const GroupFlag::Enum group1, const GroupFlag::Enum group2) X_FINAL;
	void SetGroupCollisionFlag(const GroupFlag::Enum group1, const GroupFlag::Enum group2, const bool enable) X_FINAL;

	// you must pass cooked data :|
	// if you don't have cooked data use getCooking() to cook it!
	ActorHandle createConvexMesh(const QuatTransf& myTrans, const DataArr& cooked, float density, const Vec3f& scale = Vec3f::one()) X_FINAL;
	ActorHandle createTriangleMesh(const QuatTransf& myTrans, const DataArr& cooked, float density, const Vec3f& scale = Vec3f::one()) X_FINAL;
	ActorHandle createHieghtField(const QuatTransf& myTrans, const DataArr& cooked, float density, const Vec3f& heightRowColScale = Vec3f::one()) X_FINAL;
	ActorHandle createPlane(const QuatTransf& myTrans, float density) X_FINAL;
	ActorHandle createSphere(const QuatTransf& myTrans, float radius, float density) X_FINAL;
	ActorHandle createCapsule(const QuatTransf& myTrans, float radius, float halfHeight, float density) X_FINAL;
	ActorHandle createBox(const QuatTransf& myTrans, const AABB& bounds, float density) X_FINAL;

	ActorHandle createStaticPlane(const QuatTransf& myTrans) X_FINAL;
	ActorHandle createStaticSphere(const QuatTransf& myTrans, float radius) X_FINAL;
	ActorHandle createStaticCapsule(const QuatTransf& myTrans, float radius, float halfHeight) X_FINAL;
	ActorHandle createStaticBox(const QuatTransf& myTrans, const AABB& bounds) X_FINAL;
	ActorHandle createStaticTrigger(const QuatTransf& myTrans, const AABB& bounds) X_FINAL;
	// ~IPhysics

private:
	// PvdConnectionHandler
	virtual	void onPvdSendClassDescriptions(PVD::PvdConnection&) X_FINAL;
	virtual	void onPvdConnected(PVD::PvdConnection& inFactory) X_FINAL;
	virtual	void onPvdDisconnected(PVD::PvdConnection& inFactory) X_FINAL;
	// ~PvdConnectionHandler
	
	// ~PxDeletionListener
	virtual void onRelease(const physx::PxBase* observed, void* userData, physx::PxDeletionEventFlag::Enum deletionEvent) X_FINAL;
	// ~PxDeletionListener

	// PxBroadPhaseCallback
	virtual	void onObjectOutOfBounds(physx::PxShape& shape, physx::PxActor& actor) X_FINAL;
	virtual	void onObjectOutOfBounds(physx::PxAggregate& aggregate) X_FINAL;
	// ~PxBroadPhaseCallback

	// IStepperHandler
	virtual void onSubstepPreFetchResult(void) X_FINAL;
	virtual void onSubstep(float32_t dtTime) X_FINAL;
	virtual void onSubstepSetup(float dtime, physx::PxBaseTask* cont) X_FINAL;
	// ~IStepperHandler

	// PxSimulationEventCallback
	virtual void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) X_FINAL;
	virtual void onWake(physx::PxActor** actors, physx::PxU32 count) X_FINAL;
	virtual void onSleep(physx::PxActor** actors, physx::PxU32 count) X_FINAL;
	virtual void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs) X_FINAL;
	virtual void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) X_FINAL;
	// ~PxSimulationEventCallback

	void togglePvdConnection(void);
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
	PhysxArenaAllocator	allocator_;
	PhysxLogger logger_;

	physx::PxFoundation*			pFoundation_;
	physx::PxProfileZoneManager*	pProfileZoneManager_;

	physx::PxPhysics*				pPhysics_;
	XScene*							pScene_;
	physx::PxMaterial*				pMaterial_;
	physx::PxDefaultCpuDispatcher*	pCpuDispatcher_;
	PhysCooking*					pCooking_;

	uint8_t*	pScratchBlock_;
	size_t		scratchBlockSize_;

	bool waitForResults_;
	bool pause_;
	bool oneFrameUpdate_;
	bool _pad;

	PvdParameters pvdParams_;
	physx::PxTolerancesScale scale_;

	// Steppers
	StepperType::Enum		stepperType_;
	DebugStepper			debugStepper_;
	FixedStepper			fixedStepper_;
	InvertedFixedStepper	invertedFixedStepper_;
	VariableStepper			variableStepper_;

	PhysXVars vars_;

	DebugRender* pDebugRender_;
};


X_NAMESPACE_END

#include "XPhysics.inl"