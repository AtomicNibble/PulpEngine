#include "stdafx.h"
#include "XPhysics.h"
#include "MathHelpers.h"

#include <IConsole.h>
#include <IRender.h>

#include <pvd\PxVisualDebugger.h>
#include <common\windows\PxWindowsDelayLoadHook.h>
#include <characterkinematic\PxControllerManager.h>

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


XPhysics::PvdParameters::PvdParameters() :
	ip("127.0.0.1"),
	port(5425),
	timeout(10),
	useFullPvdConnection(true)
{
}

// ---------------------------------

XPhysics::XPhysics(uint32_t maxSubSteps, core::V2::JobSystem* pJobSys, core::MemoryArenaBase* arena) :
	arena_(arena),
	jobDispatcher_(*pJobSys),
	allocator_(arena),
	pFoundation_(nullptr),
	pProfileZoneManager_(nullptr),
	pControllerManager_(nullptr),
	pPhysics_(nullptr),
	pCooking_(nullptr),
	pScene_(nullptr),
	pMaterial_(nullptr),
	pCpuDispatcher_(nullptr),
	pScratchBlock_(nullptr),
	scratchBlockSize_(0),
	initialDebugRender_(false),
	waitForResults_(false),
	pause_(false),
	oneFrameUpdate_(false),
	debugRenderScale_(1.f),
	stepperType_(StepperType::FIXED_STEPPER),

	debugStepper_(0.016666660f),
	fixedStepper_(0.016666660f, maxSubSteps),
	invertedFixedStepper_(0.016666660f, maxSubSteps),
	variableStepper_(1.0f / 80.0f, 1.0f / 40.0f, maxSubSteps),

	pDebugRender_(nullptr)

{
	X_ASSERT_NOT_NULL(arena);

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
void XPhysics::registerVars(void)
{
	vars_.RegisterVars();
}

void XPhysics::registerCmds(void)
{
	ADD_COMMAND_MEMBER("phys_toggle_pvd", this, XPhysics, &XPhysics::cmd_TogglePvd, core::VarFlag::SYSTEM,
		"Toggles PVD connection");

	ADD_COMMAND_MEMBER("phys_toggle_pause", this, XPhysics, &XPhysics::cmd_TogglePause, core::VarFlag::SYSTEM,
		"Toggles pausing physics simulation");

	ADD_COMMAND_MEMBER("phys_step_one", this, XPhysics, &XPhysics::cmd_StepOne, core::VarFlag::SYSTEM,
		"Steps one frame in the simulation");

	ADD_COMMAND_MEMBER("phys_toggle_visualization", this, XPhysics, &XPhysics::cmd_ToggleVis, core::VarFlag::SYSTEM,
		"Toggles physics visualization");
}

bool XPhysics::init(void)
{
	X_LOG0("PhysicsSys", "Starting");

	physx::PxSetPhysXDelayLoadHook(&gDelayLoadHook);
	physx::PxSetPhysXCookingDelayLoadHook(&gDelayLoadHook);

	pFoundation_ = PxCreateFoundation(PX_PHYSICS_VERSION, 
#if PHYSX_DEFAULT_ALLOCATOR
		gDefaultAllocatorCallback,
#else
		allocator_,
#endif // !PHYSX_DEFAULT_ALLOCATOR
		logger_
	);

	if (!pFoundation_) {
		X_ERROR("Physics", "Failed to create foundation");
		return false;
	}


#if !X_SUPER
	pProfileZoneManager_ = &physx::PxProfileZoneManager::createProfileZoneManager(pFoundation_);
	if (!pProfileZoneManager_) {
		X_ERROR("Physics", "Failed to create profile zone manager");
		return false;
	}
#endif // !X_SUPER

	bool recordMemoryAllocations = true;

	physx::PxTolerancesScale scale;
	customizeTolerances(scale);


	pPhysics_ = PxCreatePhysics(PX_PHYSICS_VERSION, *pFoundation_,
		scale, recordMemoryAllocations, pProfileZoneManager_);

	if (!pPhysics_) {
		X_ERROR("Physics", "PxCreatePhysics failed!");
		return false;
	}	


	if (!PxInitExtensions(*pPhysics_)) {
		X_ERROR("Physics", "Failed to init extensions");
		return false;
	}

	physx::PxCookingParams params(scale);
	params.meshWeldTolerance = 0.001f;
	params.meshPreprocessParams = physx::PxMeshPreprocessingFlags(
		physx::PxMeshPreprocessingFlag::eWELD_VERTICES |
		physx::PxMeshPreprocessingFlag::eREMOVE_UNREFERENCED_VERTICES |
		physx::PxMeshPreprocessingFlag::eREMOVE_DUPLICATED_TRIANGLES);

	pCooking_ = PxCreateCooking(PX_PHYSICS_VERSION, *pFoundation_, params);
	if (!pCooking_) {
		X_ERROR("Physics", "PxCreateCooking failed!");
		return false;
	}


	if (pPhysics_->getPvdConnectionManager()) {
		pPhysics_->getPvdConnectionManager()->addHandler(*this);
	}

	togglePvdConnection();

	pPhysics_->registerDeletionListener(*this, physx::PxDeletionEventFlag::eUSER_RELEASE);


	pMaterial_ = pPhysics_->createMaterial(0.5f, 0.5f, 0.1f);
	if (!pMaterial_) {
		X_ERROR("Physics", "Failed to create material");
		return false;
	}


	physx::PxSceneDesc sceneDesc(pPhysics_->getTolerancesScale());
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

	pScene_ = pPhysics_->createScene(sceneDesc);
	if (!pScene_) {
		X_ERROR("PhysicsSys", "Failed to create scene");
		return false;
	}

	physx::PxSceneWriteLock scopedLock(*pScene_);
	pScene_->setVisualizationParameter(physx::PxVisualizationParameter::eSCALE, initialDebugRender_ ? debugRenderScale_ : 0.0f);
	pScene_->setVisualizationParameter(physx::PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f);


	pControllerManager_ = PxCreateControllerManager(*pScene_);
	if (!pControllerManager_) {
		X_ERROR("PhysicsSys", "Failed to create controller manager");
		return false;
	}


	setScratchBlockSize(vars_.ScratchBufferSize());
	stepperType_ = vars_.GetStepperType();

	return true;
}

bool XPhysics::initRenderResources(void)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pRender);

	X_ASSERT(!pDebugRender_, "Debug render already init")(pDebugRender_);

	render::IRenderAux* pAux = gEnv->pRender->getAuxRender(render::IRender::AuxRenderer::PHYSICS);
	pDebugRender_ = X_NEW(DebugRender, arena_, "PhysDebugRender")(pAux);

	return true;
}

void XPhysics::shutDown(void)
{
	X_LOG0("PhysicsSys", "Shutting Down");

	if (pDebugRender_) {
		X_DELETE(pDebugRender_, arena_);
	}

	debugStepper_.shutdown();
	fixedStepper_.shutdown();
	invertedFixedStepper_.shutdown();
	variableStepper_.shutdown();

	if (pScene_) {
		pScene_->fetchResults(true);
	}
	if (pPhysics_) {
		pPhysics_->unregisterDeletionListener(*this);
	}

	core::SafeRelease(pControllerManager_);

	core::SafeRelease(pScene_);
	core::SafeRelease(pCpuDispatcher_);
	core::SafeRelease(pMaterial_);
	core::SafeRelease(pCooking_);

	PxCloseExtensions();

	core::SafeRelease(pPhysics_);
	core::SafeRelease(pProfileZoneManager_);
	core::SafeRelease(pFoundation_);
}

void XPhysics::release(void)
{
	X_DELETE(this, g_PhysicsArena);
}


void XPhysics::onTickPreRender(float dtime)
{
	stepperType_ = vars_.GetStepperType();

	if (oneFrameUpdate_) {
		pause_ = false;
	}

	if (!IsPaused())
	{
		Stepper* pStepper = getStepper();

		waitForResults_ = false;

		if (pScene_)
		{

			waitForResults_ = pStepper->advance(pScene_, dtime, pScratchBlock_,
				safe_static_cast<uint32_t, size_t>(scratchBlockSize_));

			// tells the stepper shape data is not going to be accessed until next frame 
			// (frame ends with stepper->wait(mScene))
			pStepper->renderDone();


		}
	}
}


void XPhysics::onTickPostRender(float dtime)
{
	if (!IsPaused() && pScene_ && waitForResults_)
	{
		Stepper* pStepper = getStepper();
		pStepper->wait(pScene_);

		core::TimeVal simTime = pStepper->getSimulationTime();

		X_LOG0("Phys", "Sim time: %gms", simTime.GetMilliSeconds());

		if (stepperType_ == StepperType::INVERTED_FIXED_STEPPER) {
			pStepper->postRender(dtime);
		}
	}

	// debug Vis
	if (pScene_ && vars_.DebugDrawEnabled() && pDebugRender_)
	{
		const physx::PxRenderBuffer& debugRenderable = pScene_->getRenderBuffer();

		pDebugRender_->clear();
		pDebugRender_->update(debugRenderable);

		updateRenderObjectsDebug(dtime);
	}

	// pause if one frame update.
	if (oneFrameUpdate_)
	{
		oneFrameUpdate_ = false;
		if (!IsPaused()) {
			togglePause();
		}
	}
}

void XPhysics::render(void)
{
	if (vars_.DebugDrawEnabled() && pDebugRender_)
	{
		pDebugRender_->queueForRender();
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
	pPhysics_->getVisualDebugger()->setVisualizeConstraints(true);
	pPhysics_->getVisualDebugger()->setVisualDebuggerFlag(physx::PxVisualDebuggerFlag::eTRANSMIT_CONTACTS, true);
	pPhysics_->getVisualDebugger()->setVisualDebuggerFlag(physx::PxVisualDebuggerFlag::eTRANSMIT_SCENEQUERIES, true);
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
	if (!pPhysics_->getPvdConnectionManager())
		return;

	if (pPhysics_->getPvdConnectionManager()->isConnected()) {
		pPhysics_->getPvdConnectionManager()->disconnect();
	}
	else {
		createPvdConnection();
	}
}

void XPhysics::createPvdConnection(void)
{
	physx::PxVisualDebuggerConnectionManager* pvd = pPhysics_->getPvdConnectionManager();
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


// --------------------------------------------------------

void XPhysics::getDefaultSceneDesc(physx::PxSceneDesc&)
{

}

void XPhysics::customizeSceneDesc(physx::PxSceneDesc&)
{

}

void XPhysics::customizeTolerances(physx::PxTolerancesScale&)
{

}

// --------------------------------------------------------


void XPhysics::updateRenderObjectsDebug(float dtime)
{
	X_UNUSED(dtime);
}

void XPhysics::updateRenderObjectsSync(float dtime)
{
	X_UNUSED(dtime);
}

void XPhysics::updateRenderObjectsAsync(float dtime)
{
	X_UNUSED(dtime);
}



Stepper* XPhysics::getStepper(void)
{
	switch (stepperType_)
	{
	case StepperType::DEBUG_STEPPER:
		return &debugStepper_;
	case StepperType::FIXED_STEPPER:
		return &fixedStepper_;
	case StepperType::INVERTED_FIXED_STEPPER:
		return &invertedFixedStepper_;
	case StepperType::VARIABLE_STEPPER:
		return &variableStepper_;
	default:
		return &variableStepper_;
	};
}


void XPhysics::setScratchBlockSize(size_t size)
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


void XPhysics::toggleVisualizationParam(physx::PxVisualizationParameter::Enum param)
{
	if (pScene_) {
		physx::PxSceneWriteLock scopedLock(*pScene_);
		const bool visualization = pScene_->getVisualizationParameter(param) == 1.0f;
		pScene_->setVisualizationParameter(param, visualization ? 0.0f : 1.0f);
	}
}

void XPhysics::setVisualizationCullingBox(AABB& box)
{
	if (pScene_) {

		physx::PxBounds3 bounds;
		bounds.minimum = Px3FromVec3(box.min);
		bounds.maximum = Px3FromVec3(box.max);

		pScene_->setVisualizationCullingBox(bounds);
		pScene_->setVisualizationParameter(physx::PxVisualizationParameter::eCULL_BOX, 1.0f);
	}
}

void XPhysics::cmd_TogglePvd(core::IConsoleCmdArgs* pArgs)
{
	X_UNUSED(pArgs);

	togglePvdConnection();
}


void XPhysics::cmd_TogglePause(core::IConsoleCmdArgs* pArgs)
{
	X_UNUSED(pArgs);

	togglePause();
}


void XPhysics::cmd_StepOne(core::IConsoleCmdArgs* pArgs)
{
	X_UNUSED(pArgs);

	oneFrameUpdate_ = true;
}

void XPhysics::cmd_ToggleVis(core::IConsoleCmdArgs* pArgs)
{
	X_UNUSED(pArgs);

	toggleVisualizationParam(physx::PxVisualizationParameter::eSCALE);
}


X_NAMESPACE_END