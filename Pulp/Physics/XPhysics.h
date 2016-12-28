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

class XPhysics : public IPhysics,
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

public:
	XPhysics(uint32_t maxSubSteps, core::V2::JobSystem* pJobSys, core::MemoryArenaBase* arena);
	~XPhysics() X_OVERRIDE;

	// IPhysics
	void registerVars(void) X_FINAL;
	void registerCmds(void) X_FINAL;

	bool init(void) X_FINAL;
	bool initRenderResources(void) X_FINAL;
	void shutDown(void) X_FINAL;
	void release(void) X_FINAL;

	void onTickPreRender(float dtime) X_FINAL;
	void onTickPostRender(float dtime) X_FINAL;
	void render(void) X_FINAL;
	// ~IPhysics
	
	MaterialHandle createMaterial(MaterialDesc& desc) X_FINAL;
	RegionHandle addRegion(const AABB& bounds) X_FINAL;

	TriggerHandle createStaticTrigger(const QuatTransf& myTrans, const AABB& bounds) X_FINAL;

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


	void customizeTolerances(physx::PxTolerancesScale&);

	void updateRenderObjectsDebug(float dtime); // update of render actors debug draw information, will be called while the simulation is NOT running
	void updateRenderObjectsSync(float dtime);  // update of render objects while the simulation is NOT running (for particles, cloth etc. because data is not double buffered)
	void updateRenderObjectsAsync(float dtime); // update of render objects, potentially while the simulation is running (for rigid bodies etc. because data is double buffered)

	Stepper* getStepper(void);

	void setScratchBlockSize(size_t size);
	void toggleVisualizationParam(physx::PxVisualizationParameter::Enum param);
	void setVisualizationCullingBox(AABB& box);

private:
	X_INLINE bool IsPaused(void) const;
	X_INLINE void togglePause(void);

	X_INLINE void setSubStepper(const float32_t stepSize, const uint32_t maxSteps);

private:
	void cmd_TogglePvd(core::IConsoleCmdArgs* pArgs);
	void cmd_TogglePause(core::IConsoleCmdArgs* pArgs);
	void cmd_StepOne(core::IConsoleCmdArgs* pArgs);
	void cmd_ToggleVis(core::IConsoleCmdArgs* pArgs);

private:
	core::MemoryArenaBase* arena_;

	PhysxCpuDispacher jobDispatcher_;
	PhysxArenaAllocator	allocator_;
	PhysxLogger logger_;

	physx::PxFoundation*			pFoundation_;
	physx::PxProfileZoneManager*	pProfileZoneManager_;
	physx::PxControllerManager*		pControllerManager_;

	physx::PxPhysics*				pPhysics_;
	physx::PxCooking*				pCooking_;
	physx::PxScene*					pScene_;
	physx::PxMaterial*				pMaterial_;
	physx::PxDefaultCpuDispatcher*	pCpuDispatcher_;

	uint8_t*	pScratchBlock_;
	size_t		scratchBlockSize_;

	bool initialDebugRender_;
	bool waitForResults_;
	bool pause_;
	bool oneFrameUpdate_;
	physx::PxReal debugRenderScale_;

	PvdParameters pvdParams_;

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