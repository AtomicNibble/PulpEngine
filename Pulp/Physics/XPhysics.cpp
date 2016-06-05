#include "stdafx.h"
#include "XPhysics.h"


#include <pvd\PxVisualDebugger.h>
#include <common\windows\PxWindowsDelayLoadHook.h>

#if X_DEBUG
#define PHYS_LIB_SUFFIX "DEBUG"
#elif X_RELEASE
#define PHYS_LIB_SUFFIX "CHECKED"
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
#if PHYSX_DEFAULT_ALLOCATOR
	physx::PxDefaultAllocator gDefaultAllocatorCallback;
#endif // !PHYSX_DEFAULT_ALLOCATOR


	class SampleDelayLoadHook : public physx::PxDelayLoadHook
	{
	public:

		virtual const char* getPhysXCommonDEBUGDllName(void) const X_FINAL
		{
			return "PhysXCommonDEBUG.dll";
		}
		virtual const char* getPhysXCommonCHECKEDDllName(void) const X_FINAL
		{
			return "PhysXCommonCHECKED.dll";
		}
		virtual const char* getPhysXCommonPROFILEDllName(void) const X_FINAL
		{
			return "PhysXCommonPROFILE.dll";
		}
		virtual const char* getPhysXCommonDllName(void) const X_FINAL
		{
			return "PhysXCommon.dll";
		}
	} gDelayLoadHook;

} // namespace


XPhysics::XPhysics(uint32_t maxSubSteps, core::V2::JobSystem* pJobSys, core::MemoryArenaBase* arena) :
	jobDispatcher_(*pJobSys),
	allocator_(arena),
	foundation_(nullptr),
	profileZoneManager_(nullptr),
	physics_(nullptr),
	cooking_(nullptr),
	scene_(nullptr),
	material_(nullptr),
	cpuDispatcher_(nullptr),
	pScratchBlock_(nullptr),
	scratchBlockSize_(0),
	initialDebugRender_(false),
	debugRenderScale_(1.f),
	stepperType_(StepperType::FIXED_STEPPER),

	debugStepper_(0.016666660f),
	fixedStepper_(0.016666660f, maxSubSteps),
	invertedFixedStepper_(0.016666660f, maxSubSteps),
	variableStepper_(1.0f / 80.0f, 1.0f / 40.0f, maxSubSteps)
{
	X_ASSERT_NOT_NULL(arena);

	setSctrachBlock(SCRATCH_BLOCK_SIZE);

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
void XPhysics::RegisterVars(void)
{


}

void XPhysics::RegisterCmds(void)
{


}

bool XPhysics::Init(void)
{
	X_LOG0("PhysicsSys", "Starting");

	physx::PxSetPhysXDelayLoadHook(&gDelayLoadHook);
	physx::PxSetPhysXCookingDelayLoadHook(&gDelayLoadHook);

	foundation_ = PxCreateFoundation(PX_PHYSICS_VERSION, 
#if PHYSX_DEFAULT_ALLOCATOR
		gDefaultAllocatorCallback,
#else
		allocator_,
#endif // !PHYSX_DEFAULT_ALLOCATOR
		logger_
	);

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
	customizeTolerances(scale);


	physics_ = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation_,
		scale, recordMemoryAllocations, profileZoneManager_);

	if (!physics_) {
		X_ERROR("Physics", "PxCreatePhysics failed!");
		return false;
	}	


	if (!PxInitExtensions(*physics_)) {
		X_ERROR("Physics", "Failed to init extensions");
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

	togglePvdConnection();

	physics_->registerDeletionListener(*this, physx::PxDeletionEventFlag::eUSER_RELEASE);


	material_ = physics_->createMaterial(0.5f, 0.5f, 0.1f);
	if (!material_) {
		X_ERROR("Physics", "Failed to create material");
		return false;
	}


	physx::PxSceneDesc sceneDesc(physics_->getTolerancesScale());
	sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
	getDefaultSceneDesc(sceneDesc);
	customizeSceneDesc(sceneDesc);

	sceneDesc.cpuDispatcher = &jobDispatcher_;

	if (!sceneDesc.filterShader) {
		sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
	}



	//sceneDesc.flags |= physx::PxSceneFlag::eENABLE_TWO_DIRECTIONAL_FRICTION;
	//sceneDesc.flags |= physx::PxSceneFlag::eENABLE_PCM;
	//sceneDesc.flags |= physx::PxSceneFlag::eENABLE_ONE_DIRECTIONAL_FRICTION;  
	//sceneDesc.flags |= physx::PxSceneFlag::eADAPTIVE_FORCE;
	sceneDesc.flags |= physx::PxSceneFlag::eENABLE_ACTIVETRANSFORMS;
	//sceneDesc.flags |= physx::PxSceneFlag::eDISABLE_CONTACT_CACHE;


	if (stepperType_ == StepperType::INVERTED_FIXED_STEPPER) {
		sceneDesc.simulationOrder = physx::PxSimulationOrder::eSOLVE_COLLIDE;
	}

	scene_ = physics_->createScene(sceneDesc);
	if (!scene_) {
		X_ERROR("PhysicsSys", "Failed to create scene");
		return false;
	}

	physx::PxSceneWriteLock scopedLock(*scene_);
	scene_->setVisualizationParameter(physx::PxVisualizationParameter::eSCALE, initialDebugRender_ ? debugRenderScale_ : 0.0f);
	scene_->setVisualizationParameter(physx::PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f);


	return true;
}

void XPhysics::ShutDown(void)
{
	X_LOG0("PhysicsSys", "Shutting Down");

	debugStepper_.shutdown();
	fixedStepper_.shutdown();
	invertedFixedStepper_.shutdown();
	variableStepper_.shutdown();

	if (scene_) {
		scene_->fetchResults(true);
	}
	if (physics_) {
		physics_->unregisterDeletionListener(*this);
	}

	core::SafeRelease(scene_);
	core::SafeRelease(cpuDispatcher_);
	core::SafeRelease(material_);
	core::SafeRelease(cooking_);

	PxCloseExtensions();

	core::SafeRelease(physics_);
	core::SafeRelease(profileZoneManager_);
	core::SafeRelease(foundation_);
}

void XPhysics::release(void)
{
	X_DELETE(this, g_PhysicsArena);
}


void XPhysics::onTickPreRender(float dtime)
{
	Stepper* pStepper = getStepper();

	waitForResults_ = false;

	if (scene_)
	{

		waitForResults_ = pStepper->advance(scene_, dtime, pScratchBlock_, 
			safe_static_cast<uint32_t, size_t>(scratchBlockSize_));

		// tells the stepper shape data is not going to be accessed until next frame 
		// (frame ends with stepper->wait(mScene))
		pStepper->renderDone();


	}
}


void XPhysics::onTickPostRender(float dtime)
{
	if (scene_ && waitForResults_)
	{
		Stepper* pStepper = getStepper();
		pStepper->wait(scene_);


		if (stepperType_ == StepperType::INVERTED_FIXED_STEPPER) {
			pStepper->postRender(dtime);
		}
	}
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


void XPhysics::togglePvdConnection(void)
{
	if (!physics_->getPvdConnectionManager())
		return;

	if (physics_->getPvdConnectionManager()->isConnected()) {
		physics_->getPvdConnectionManager()->disconnect();
	}
	else {
		createPvdConnection();
	}
}

void XPhysics::createPvdConnection(void)
{
	physx::PxVisualDebuggerConnectionManager* pvd = physics_->getPvdConnectionManager();
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


void XPhysics::getDefaultSceneDesc(physx::PxSceneDesc&)
{

}

void XPhysics::customizeSceneDesc(physx::PxSceneDesc&)
{

}

void XPhysics::customizeTolerances(physx::PxTolerancesScale&)
{


}


Stepper* XPhysics::getStepper(void)
{
	switch (stepperType_)
	{
	case StepperType::DEFAULT_STEPPER:
		return &debugStepper_;
	case StepperType::FIXED_STEPPER:
		return &fixedStepper_;
	case StepperType::INVERTED_FIXED_STEPPER:
		return &invertedFixedStepper_;
	default:
		return &variableStepper_;
	};

}


void XPhysics::setSctrachBlock(size_t size)
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


X_NAMESPACE_END