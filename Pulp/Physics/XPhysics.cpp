#include "stdafx.h"
#include "XPhysics.h"


#include <pvd\PxVisualDebugger.h>

#if X_DEBUG
#define PHYS_LIB_SUFFIX "DEBUG"
#elif X_RELEASE
#define PHYS_LIB_SUFFIX "PROFILE"
#elif X_SUPER
#define PHYS_LIB_SUFFIX "" // no suffix.
#else
#error "Unkown config"
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
	physx::PxDefaultAllocator gDefaultAllocatorCallback;
	physx::PxDefaultErrorCallback gDefaultErrorCallback;

} // namespace


XPhysics::XPhysics() :
	foundation_(nullptr),
	profileZoneManager_(nullptr),
	physics_(nullptr),
	cooking_(nullptr),
	scene_(nullptr)
{

}

XPhysics::~XPhysics()
{


}

// IPhysics
void XPhysics::RegisterVars(void)
{


}

void XPhysics::RegisterCmds(void)
{


}

bool XPhysics::Init(void)
{
	X_LOG0("PhysicsSys", "Starting");

	foundation_ = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocatorCallback, gDefaultErrorCallback);
	if (!foundation_) {
		X_ERROR("Physics", "Failed to create foundation");
		return false;
	}


#if !X_SUPER
	profileZoneManager_ = &physx::PxProfileZoneManager::createProfileZoneManager(foundation_);
	if (!profileZoneManager_) {
		X_ERROR("Physics", "Failed to create profile zone manager");
		return false;
	}
#endif // !X_SUPER

	bool recordMemoryAllocations = true;

	physx::PxTolerancesScale scale;
	//customizeTolerances(scale);


	physics_ = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation_,
		scale, recordMemoryAllocations, profileZoneManager_);

	if (!physics_) {
		X_ERROR("Physics", "PxCreatePhysics failed!");
		return false;
	}	


	physx::PxCookingParams params(scale);
	params.meshWeldTolerance = 0.001f;
	params.meshPreprocessParams = physx::PxMeshPreprocessingFlags(
		physx::PxMeshPreprocessingFlag::eWELD_VERTICES |
		physx::PxMeshPreprocessingFlag::eREMOVE_UNREFERENCED_VERTICES |
		physx::PxMeshPreprocessingFlag::eREMOVE_DUPLICATED_TRIANGLES);

	cooking_ = PxCreateCooking(PX_PHYSICS_VERSION, *foundation_, params);
	if (!cooking_) {
		X_ERROR("Physics", "PxCreateCooking failed!");
		return false;
	}


	if (physics_->getPvdConnectionManager()) {
		physics_->getPvdConnectionManager()->addHandler(*this);
	}

	physics_->registerDeletionListener(*this, physx::PxDeletionEventFlag::eUSER_RELEASE);




	return true;
}

void XPhysics::ShutDown(void)
{
	X_LOG0("PhysicsSys", "Shutting Down");

	if (scene_) {
		scene_->fetchResults(true);
	}


	core::SafeRelease(scene_);
	core::SafeRelease(cooking_);
	core::SafeRelease(physics_);
	core::SafeRelease(profileZoneManager_);
	core::SafeRelease(foundation_);
}

void XPhysics::release(void)
{
	X_DELETE(this, g_PhysicsArena);
}


void XPhysics::onPvdSendClassDescriptions(PVD::PvdConnection& conn)
{
	X_UNUSED(conn);

}

void XPhysics::onPvdConnected(PVD::PvdConnection& conn)
{
	X_UNUSED(conn);

	//setup joint visualization.  This gets piped to pvd.
	physics_->getVisualDebugger()->setVisualizeConstraints(true);
	physics_->getVisualDebugger()->setVisualDebuggerFlag(physx::PxVisualDebuggerFlag::eTRANSMIT_CONTACTS, true);
	physics_->getVisualDebugger()->setVisualDebuggerFlag(physx::PxVisualDebuggerFlag::eTRANSMIT_SCENEQUERIES, true);
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


X_NAMESPACE_END