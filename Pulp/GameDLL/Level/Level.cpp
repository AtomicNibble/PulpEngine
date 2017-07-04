#include "stdafx.h"
#include "Level.h"
#include "Enity\EnitiySystem.h"


#include <Threading\JobSystem2.h>

#include <IFileSys.h>
#include <IFrameData.h>

X_NAMESPACE_BEGIN(game)

Level::Level(core::MemoryArenaBase* arena, entity::EnititySystem& entSys) :
	arena_(arena),
	stringTable_(arena),
	entSys_(entSys)
{
	pJobSys_ = X_ASSERT_NOT_NULL(gEnv->pJobSys);
	pFileSys_ = X_ASSERT_NOT_NULL(gEnv->pFileSys);

}

Level::~Level()
{

}



void Level::load(const char* mapName)
{
	path_.set(mapName);
	path_.setExtension(level::LVL_FILE_EXTENSION);

	free();

	X_LOG0("Level", "Loading level: %s", mapName);

	core::IoRequestOpen open;
	open.callback.Bind<Level, &Level::IoRequestCallback>(this);
	open.mode = core::fileMode::READ;
	open.path = path_;

	pFileSys_->AddIoRequestToQue(open);
}

void Level::free(void)
{
	core::zero_object(fileHdr_);

	levelData_.reset();

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
		free();
	}

	core::IoRequestClose req;
	req.pFile = pFile;
	pFileSys_->AddIoRequestToQue(req);
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
	pScene_(nullptr),
	ents_(arena),
	level_(arena, ents_)
{

}

World::~World()
{

}

bool World::init(physics::IPhysics* pPhys)
{
	if (!createPhysicsScene(pPhys)) {
		return false;
	}

	if (!ents_.init(pPhys, pScene_)) {
		return false;
	}

	ents_.createPlayer(Vec3f(-128.f, 0.f, 200.f));

	level_.load("physics_test");

	return true;
}

void World::update(core::FrameData& frame)
{

	ents_.update(frame);

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

	gEnv->pPhysicsScene = pScene_;

	return true;
}


X_NAMESPACE_END