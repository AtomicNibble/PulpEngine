#include "stdafx.h"
#include "Level.h"
#include "Enity\EnitiySystem.h"

#include <Threading\JobSystem2.h>

#include <I3DEngine.h>
#include <IWorld3D.h>
#include <IFileSys.h>
#include <IFrameData.h>

#include "UserCmds\UserCmdMan.h"

using namespace sound::Literals;

X_NAMESPACE_BEGIN(game)

Level::Level(physics::IScene* pScene, engine::IWorld3D* p3DWorld,
    entity::EnititySystem& entSys, core::MemoryArenaBase* arena) :
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

void Level::beginLoad(const core::string& name)
{
    path_.set(name);
    path_.setExtension(level::LVL_FILE_EXTENSION);

    clear();

    X_LOG0("Level", "Loading level: %s", name.c_str());

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

    if (requestType == core::IoRequest::OPEN) {
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
    else if (requestType == core::IoRequest::READ) {
        if (!bytesTransferred) {
            X_ERROR("Level", "Failed to read level data");
            return;
        }

        const core::IoRequestRead* pReadReq = static_cast<const core::IoRequestRead*>(pRequest);

        if (pReadReq->pBuf == &fileHdr_) {
            pJobSys_->CreateMemberJobAndRun<Level>(this, &Level::processHeader_job, pFile JOB_SYS_SUB_ARG(core::profiler::SubSys::GAME));
        }
        else {
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
    if (!processHeader()) {
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

    if (!processData()) {
        X_ERROR("Level", "Failed to load process data");
        clear();
    }

    if (!p3DWorld_->loadNodes(fileHdr_, stringTable_, levelData_.ptr())) {
        X_ERROR("Level", "Failed to load process 3d data");
        clear();
    }

    if (!processEnts()) {
        X_ERROR("Level", "Failed to process ent data");
        clear();
    }

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
    if (!fileHdr_.isValid()) {
        // this header is not valid :Z
        if (fileHdr_.fourCC == level::LVL_FOURCC_INVALID) {
            X_ERROR("Level", "%s file is corrupt, please re-compile.", path_.fileName());
            return false;
        }

        X_ERROR("Level", "%s is not a valid level", path_.fileName());
        return false;
    }

    if (fileHdr_.version != level::LVL_VERSION) {
        X_ERROR("Level", "%s has a invalid version. provided: %i required: %i",
            path_.fileName(), fileHdr_.version, level::LVL_VERSION);
        return false;
    }

    if (fileHdr_.totalDataSize <= 0) {
        X_ERROR("Level", "level file is empty");
        return false;
    }

    // require atleast one area.
    if (fileHdr_.numAreas < 1) {
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

        if (!stringTable_.SLoad(&file)) {
            X_ERROR("Level", "Failed to load string table.");
            return false;
        }

        if (!file.isEof()) {
            X_ERROR("Level", "Failed to fully parse sting table.");
            return false;
        }
    }

    return true;
}

bool Level::processEnts(void)
{
    {
        core::XFileFixedBuf file = fileHdr_.FileBufForNode(levelData_.ptr(), level::FileNodes::ENTITIES);

        entSys_.loadEntites(
            reinterpret_cast<const char*>(file.getBufferStart()),
            reinterpret_cast<const char*>(file.getBufferEnd()));
    }

    core::XFileMemScoped entFile;

    if (!entFile.openFile("entdesc.json", core::fileMode::READ)) {
        return false;
    }

    entSys_.loadEntites2(
        reinterpret_cast<const char*>(entFile->getBufferStart()),
        reinterpret_cast<const char*>(entFile->getBufferEnd()));

    entSys_.postLoad();

    return true;
}

// -------------------------------------

World::World(GameVars& vars, physics::IPhysics* pPhys,
    UserCmdMan& userCmdMan, game::weapon::WeaponDefManager& weaponDefs, core::MemoryArenaBase* arena) :
    arena_(arena),
    pPhys_(pPhys),
    pScene_(nullptr),
    ents_(vars, weaponDefs, arena),
    level_(arena),
    userCmdMan_(userCmdMan)
{
}

World::~World()
{
}

bool World::loadMap(const core::string& name)
{
    X_LOG0("Game", "Loading map: \"%s\"", name.c_str());

    if (!createPhysicsScene(pPhys_)) {
        return false;
    }

    gEnv->pSound->setPhysicsScene(pScene_);

    auto* pWorld3D = gEnv->p3DEngine->create3DWorld(pScene_);

    if (!ents_.init(pPhys_, pScene_, pWorld3D)) {
        return false;
    }

    level_ = core::makeUnique<Level>(arena_, pScene_, pWorld3D, ents_, arena_);
    level_->beginLoad(name);

    // TEMP
    // gEnv->pSound->postEvent(force_hash<"play_ambient"_soundId>(), sound::GLOBAL_OBJECT_ID);
    // gEnv->pSound->postEvent(force_hash<"video_rickroll"_soundId>(), sound::GLOBAL_OBJECT_ID);

    return true;
}

bool World::hasLoaded(void) const
{
    X_ASSERT(level_, "Level not valid")();

    return level_->isLoaded();
}

void World::update(core::FrameData& frame, UserCmdMan& userCmdMan)
{
    X_UNUSED(userCmdMan);

    X_ASSERT(level_ && level_->isLoaded(), "Level not valid")();

    static bool spawn = false;

    if (!spawn) {
        spawn = true;
        spawnPlayer(0);
    }

    ents_.update(frame, userCmdMan);

    level_->update(frame);
}

void World::spawnPlayer(entity::EntityId id)
{
    X_ASSERT(id < MAX_PLAYERS, "Invalide player id")(id);
    X_ASSERT_NOT_NULL(pScene_);

    // we want to take the give ent and set it up as a player.
    // the ent may not have all the required components yet.
    // we also want to support spawning diffrent types of players with various definitions.
    // maybe i should write some system for defining what each enity type has.
    // for now can be just in code, but setup in way that can be data driven.
    // which ia suspect should be handled by the enitiy system.
    // so we allmost what a make ent type helper, that we can just call and it makes the ent a player.
    // then here we can do shit with the player.

    ents_.makePlayer(id);

    auto& trans = ents_.getRegister().get<entity::TransForm>(id);
    trans.pos = Vec3f(-80, 0, 10);
    ents_.addController(id);

    userCmdMan_.resetPlayer(id);
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
