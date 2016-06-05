#pragma once

#include <IPhysics.h>

#include "extensions/PxExtensionsAPI.h"
#include "physxvisualdebuggersdk/PvdConnectionManager.h"

#include "Allocator.h"
#include "Logger.h"
#include "CpuDispatcher.h"
#include "Stepper.h"

namespace PVD {
	using namespace physx::debugger;
	using namespace physx::debugger::comm;
}


X_NAMESPACE_BEGIN(physics)

class XPhysics : public IPhysics,
	public PVD::PvdConnectionHandler, //receive notifications when pvd is connected and disconnected.
	public physx::PxDeletionListener,
	public IStepperHandler
{
	X_DECLARE_ENUM(StepperType) (
		DEFAULT_STEPPER,
		FIXED_STEPPER,
		INVERTED_FIXED_STEPPER,
		VARIABLE_STEPPER
	);

	struct PvdParameters
	{
		PvdParameters() :
			ip("127.0.0.1"),
			port(5425),
			timeout(10),
			useFullPvdConnection(true)
		{
		}

		core::StackString<128>	ip;
		uint32_t	port;
		uint32_t	timeout;
		bool	useFullPvdConnection;
	};

	static const size_t SCRATCH_BLOCK_SIZE = 1024 * 16;

	X_NO_COPY(XPhysics);
	X_NO_ASSIGN(XPhysics);

public:
	XPhysics(uint32_t maxSubSteps, core::V2::JobSystem* pJobSys, core::MemoryArenaBase* arena);
	~XPhysics() X_OVERRIDE;

	// IPhysics
	void RegisterVars(void) X_FINAL;
	void RegisterCmds(void) X_FINAL;

	bool Init(void) X_FINAL;
	void ShutDown(void) X_FINAL;
	void release(void) X_FINAL;

	void onTickPreRender(float dtime) X_FINAL;
	void onTickPostRender(float dtime) X_FINAL;
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

	// IStepperHandler
	virtual void onSubstepPreFetchResult(void) X_FINAL;
	virtual void onSubstep(float32_t dtTime) X_FINAL;
	virtual void onSubstepSetup(float dtime, physx::PxBaseTask* cont) X_FINAL;
	// ~IStepperHandler

	void togglePvdConnection(void);
	void createPvdConnection(void);

	void getDefaultSceneDesc(physx::PxSceneDesc&);
	void customizeSceneDesc(physx::PxSceneDesc&);
	void customizeTolerances(physx::PxTolerancesScale&);

	Stepper* getStepper(void);

	void setSctrachBlock(size_t size);

private:
	PhysxCpuDispacher jobDispatcher_;
	PhysxArenaAllocator	allocator_;
	PhysxLogger logger_;

	physx::PxFoundation*				foundation_;
	physx::PxProfileZoneManager*		profileZoneManager_;
	physx::PxPhysics*					physics_;
	physx::PxCooking*					cooking_;
	physx::PxScene*						scene_;
	physx::PxMaterial*					material_;
	physx::PxDefaultCpuDispatcher*		cpuDispatcher_;

	uint8_t* pScratchBlock_;
	size_t scratchBlockSize_;

	bool initialDebugRender_;
	bool waitForResults_;
	physx::PxReal debugRenderScale_;

	PvdParameters pvdParams_;

	// Steppers
	StepperType::Enum		stepperType_;
	DebugStepper			debugStepper_;
	FixedStepper			fixedStepper_;
	InvertedFixedStepper	invertedFixedStepper_;
	VariableStepper			variableStepper_;
};


X_NAMESPACE_END