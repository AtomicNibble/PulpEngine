#pragma once

#include <IPhysics.h>

#include "extensions/PxExtensionsAPI.h"
#include "physxvisualdebuggersdk/PvdConnectionManager.h"

#include "Allocator.h"
#include "Logger.h"
#include "CpuDispatcher.h"

namespace PVD {
	using namespace physx::debugger;
	using namespace physx::debugger::comm;
}


X_NAMESPACE_BEGIN(physics)

class XPhysics : public IPhysics,
	public PVD::PvdConnectionHandler, //receive notifications when pvd is connected and disconnected.
	public physx::PxDeletionListener
{
	X_DECLARE_ENUM(StepperType) (
		DEFAULT_STEPPER,
		FIXED_STEPPER,
		INVERTED_FIXED_STEPPER,
		VARIABLE_STEPPER
	);


	X_NO_COPY(XPhysics);
	X_NO_ASSIGN(XPhysics);

public:
	XPhysics(core::V2::JobSystem* pJobSys, core::MemoryArenaBase* arena);
	~XPhysics() X_OVERRIDE;

	// IPhysics
	void RegisterVars(void) X_FINAL;
	void RegisterCmds(void) X_FINAL;

	bool Init(void) X_FINAL;
	void ShutDown(void) X_FINAL;
	void release(void) X_FINAL;
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


	void getDefaultSceneDesc(physx::PxSceneDesc&);
	void customizeSceneDesc(physx::PxSceneDesc&);
	void customizeTolerances(physx::PxTolerancesScale&);

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

	bool initialDebugRender_;
	physx::PxReal debugRenderScale_;

	StepperType::Enum stepperType_;
};


X_NAMESPACE_END