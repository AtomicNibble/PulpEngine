#include "stdafx.h"
#include "Level.h"
#include "Enity\EnitiySystem.h"


#include <Threading\JobSystem2.h>

#include <I3DEngine.h>
#include <IWorld3D.h>
#include <IFileSys.h>
#include <IFrameData.h>



X_NAMESPACE_BEGIN(game)

Level::Level(core::MemoryArenaBase* arena, physics::IScene* pScene, 
	engine::IWorld3D* p3DWorld, entity::EnititySystem& entSys) :
	arena_(arena),
	stringTable_(arena),
	entSys_(entSys),
	loaded_(false),
	p3DWorld_(p3DWorld),
	pScene_(pScene)
{
	pJobSys_ = X_ASSERT_NOT_NULL(gEnv->pJobSys);
	pFileSys_ = X_ASSERT_NOT_NULL(gEnv->pFileSys);

}

Level::~Level()
{

}

void Level::update(core::FrameData& frame)
{
	X_UNUSED(frame);


}


void Level::load(const char* mapName)
{
	path_.set(mapName);
	path_.setExtension(level::LVL_FILE_EXTENSION);

	clear();

	X_LOG0("Level", "Loading level: %s", mapName);

	core::IoRequestOpen open;
	open.callback.Bind<Level, &Level::IoRequestCallback>(this);
	open.mode = core::fileMode::READ;
	open.path = path_;

	pFileSys_->AddIoRequestToQue(open);
}

void Level::clear(void)
{
	if (loaded_) {

		gEnv->p3DEngine->removeWorldFromActiveList(p3DWorld_);
		gEnv->pPhysics->removeSceneFromSim(pScene_);
	}


	loaded_ = false;

	core::zero_object(fileHdr_);

	levelData_.reset();
	stringTable_.free();
}


void Level::IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
	core::XFileAsync* pFile, uint32_t bytesTransferred)
{
	core::IoRequest::Enum requestType = pRequest->getType();

	if (requestType == core::IoRequest::OPEN)
	{
		if (!pFile) {
			X_ERROR("Level", "Failed to open level file");
			return;
		}

		core::IoRequestRead read;
		read.callback.Bind<Level, &Level::IoRequestCallback>(this);
		read.pFile = pFile;
		read.dataSize = sizeof(fileHdr_);
		read.offset = 0;
		read.pBuf = &fileHdr_;

		fileSys.AddIoRequestToQue(read);
	}
	else if (requestType == core::IoRequest::READ)
	{
		if (!bytesTransferred) {
			X_ERROR("Level", "Failed to read level data");
			return;
		}

		const core::IoRequestRead* pReadReq = static_cast<const core::IoRequestRead*>(pRequest);

		if (pReadReq->pBuf == &fileHdr_) 
		{
			pJobSys_->CreateMemberJobAndRun<Level>(this, &Level::processHeader_job, pFile JOB_SYS_SUB_ARG(core::profiler::SubSys::GAME));
		}
		else 
		{
			pJobSys_->CreateMemberJobAndRun<Level>(this, &Level::processData_job, pFile JOB_SYS_SUB_ARG(core::profiler::SubSys::GAME));
		}
	}
}




void Level::processHeader_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_UNUSED(jobSys);
	X_UNUSED(threadIdx);
	X_UNUSED(pJob);

	core::XFileAsync* pFile = static_cast<core::XFileAsync*>(pData);

	// check if the header is correct.
	if (!processHeader())
	{
		X_ERROR("Level", "Failed to process header");
		return;
	}


	// allocate buffer for the file data.
	levelData_ = core::makeUnique<uint8_t[]>(arena_, fileHdr_.totalDataSize, 64);

	core::IoRequestRead read;
	read.callback.Bind<Level, &Level::IoRequestCallback>(this);
	read.dataSize = fileHdr_.totalDataSize;
	read.offset = sizeof(fileHdr_);
	read.pBuf = levelData_.ptr();
	read.pFile = pFile;

	pFileSys_->AddIoRequestToQue(read);
}

void Level::processData_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_UNUSED(jobSys);
	X_UNUSED(threadIdx);
	X_UNUSED(pJob);
	X_UNUSED(pData);

	X_ASSERT(levelData_, "Level data pointer is not valid")(levelData_);

	core::XFileAsync* pFile = static_cast<core::XFileAsync*>(pData);

	if (!processData())
	{
		X_ERROR("Level", "Failed to load process data");
		clear();
	}

	if (!p3DWorld_->loadNodes(fileHdr_, stringTable_, levelData_.ptr()))
	{
		X_ERROR("Level", "Failed to load process 3d data");
		clear();
	}

#if 1
	Transformf trans;

	for (size_t i = 0; i < 30; i++)
	{
		trans.pos.x = gEnv->xorShift.randRange(-200.f, 200.f);
		trans.pos.y = gEnv->xorShift.randRange(-200.f, 200.f);
		trans.pos.z = gEnv->xorShift.randRange(20.f, 100.f);

		float size = gEnv->xorShift.randRange(5.f, 20.f);

		auto box = gEnv->pPhysics->createBox(trans, AABB(Vec3f::zero(), size), 0.5f);
		pScene_->addActorToScene(box);
	}

	for (size_t i = 0; i < 20; i++)
	{
		trans.pos.x = -248;
		trans.pos.y = 180;
		trans.pos.z = 80;

		trans.pos.x += gEnv->xorShift.randRange(-20.f, 20.f);
		trans.pos.y += gEnv->xorShift.randRange(-20.f, 20.f);


		float size = gEnv->xorShift.randRange(1.f, 10.f);

		auto box = gEnv->pPhysics->createBox(trans, AABB(Vec3f::zero(), size), 0.5f);
		pScene_->addActorToScene(box);
	}

	trans.pos.y = 120;

	for (size_t i = 0; i < 10; i++)
	{
		float size = gEnv->xorShift.randRange(1.f, 10.f);

		trans.pos.y = 120;
		trans.pos.x += gEnv->xorShift.randRange(-20.f, 20.f);
		trans.pos.y += gEnv->xorShift.randRange(-20.f, 20.f);

		auto box = gEnv->pPhysics->createSphere(trans, size, 0.6f);
		pScene_->addActorToScene(box);
	}
#endif

	core::IoRequestClose req;
	req.pFile = pFile;
	pFileSys_->AddIoRequestToQue(req);
	
//	core::Thread::Sleep(5000);

	// activate the scene and 3d world.
	gEnv->p3DEngine->addWorldToActiveList(p3DWorld_);
	gEnv->pPhysics->addSceneToSim(pScene_);

	loaded_ = true;
}



bool Level::processHeader(void)
{
	// is this header valid m'lady?
	if (!fileHdr_.isValid())
	{
		// this header is not valid :Z
		if (fileHdr_.fourCC == level::LVL_FOURCC_INVALID)
		{
			X_ERROR("Level", "%s file is corrupt, please re-compile.", path_.fileName());
			return false;
		}

		X_ERROR("Level", "%s is not a valid level", path_.fileName());
		return false;
	}

	if (fileHdr_.version != level::LVL_VERSION)
	{
		X_ERROR("Level", "%s has a invalid version. provided: %i required: %i",
			path_.fileName(), fileHdr_.version, level::LVL_VERSION);
		return false;
	}

	if (fileHdr_.totalDataSize <= 0)
	{
		X_ERROR("Level", "level file is empty");
		return false;
	}

	// require atleast one area.
	if (fileHdr_.numAreas < 1)
	{
		X_ERROR("Level", "Level file has no areas");
		return false;
	}

	return true;
}



bool Level::processData(void)
{
	// read string table.
	{
		core::XFileFixedBuf file = fileHdr_.FileBufForNode(levelData_.ptr(), level::FileNodes::STRING_TABLE);

		if (!stringTable_.SLoad(&file))
		{
			X_ERROR("Level", "Failed to load string table.");
			return false;
		}

		if (!file.isEof())
		{
			X_ERROR("Level", "Failed to fully parse sting table.");
			return false;
		}
	}

	{
		core::XFileFixedBuf file = fileHdr_.FileBufForNode(levelData_.ptr(), level::FileNodes::ENTITIES);

		entSys_.loadEntites(
			reinterpret_cast<const char*>(file.getBufferStart()), 
			reinterpret_cast<const char*>(file.getBufferEnd())
		);

	}


	return true;
}



// -------------------------------------


World::World(core::MemoryArenaBase* arena) :
	arena_(arena),
	pScene_(nullptr),
	ents_(arena),
	level_(arena)
{

}

World::~World()
{

}

bool World::init(physics::IPhysics* pPhys)
{
	pPhys_ = pPhys;

#if 0
	if (!createPhysicsScene(pPhys)) {
		return false;
	}

	if (!ents_.init(pPhys, pScene_)) {
		return false;
	}
#endif

#if 0
	auto* pWorld3D = gEnv->p3DEngine->create3DWorld(pScene_);

	level_ = core::makeUnique<Level>(arena_, arena_, pScene_, pWorld3D, ents_);
	level_->load("physics_test");


	ents_.createPlayer(Vec3f(-128.f, 0.f, 500.f));
#endif
	return true;
}

bool World::loadMap(const char* pMapName)
{
	X_LOG0("Game", "Loading map: \"%s\"", pMapName);

	if (!createPhysicsScene(pPhys_)) {
		return false;
	}

	if (!ents_.init(pPhys_, pScene_)) {
		return false;
	}

	auto* pWorld3D = gEnv->p3DEngine->create3DWorld(pScene_);

	level_ = core::makeUnique<Level>(arena_, arena_, pScene_, pWorld3D, ents_);
	level_->load(pMapName);

	ents_.createPlayer(Vec3f(-128.f, 0.f, 50.f));

	return true;
}

void World::update(core::FrameData& frame)
{


	if (level_ && level_->isLoaded()) 
	{
		ents_.update(frame);
		
		level_->update(frame);
	}

}



bool World::createPhysicsScene(physics::IPhysics* pPhys)
{
	if (pScene_) {
		pPhys->releaseScene(pScene_);
	}

	// lets make a scene.
	physics::SceneDesc sceneDesc;
	core::zero_object(sceneDesc.sceneLimitHint); // zero = no limit
	sceneDesc.gravity = Vec3f(0.f, 0.f, -9.8f);
	sceneDesc.frictionType = physics::FrictionType::Patch;
	sceneDesc.frictionOffsetThreshold = 0.04f;
	sceneDesc.contractCorrelationDis = 0.025f;
	sceneDesc.bounceThresholdVelocity = 0.2f;
	sceneDesc.sanityBounds = AABB(Vec3f(static_cast<float>(level::MIN_WORLD_COORD)),
		Vec3f(static_cast<float>(level::MAX_WORLD_COORD)));

	pScene_ = pPhys->createScene(sceneDesc);
	if (!pScene_) {
		return false;
	}

	return true;
}


X_NAMESPACE_END