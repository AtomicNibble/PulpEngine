#pragma once

#include <IPhysics.h>

#include "extensions/PxExtensionsAPI.h"
#include "physxvisualdebuggersdk/PvdConnectionManager.h"

namespace PVD {
	using namespace physx::debugger;
	using namespace physx::debugger::comm;
}


X_NAMESPACE_BEGIN(physics)

class XPhysics : public IPhysics,
	public PVD::PvdConnectionHandler, //receive notifications when pvd is connected and disconnected.
	public physx::PxDeletionListener
{
public:
	XPhysics();
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


private:
	physx::PxFoundation*				foundation_;
	physx::PxProfileZoneManager*		profileZoneManager_;
	physx::PxPhysics*					physics_;
	physx::PxCooking*					cooking_;
	physx::PxScene*						scene_;


};


X_NAMESPACE_END