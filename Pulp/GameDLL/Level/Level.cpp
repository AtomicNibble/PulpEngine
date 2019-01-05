#include "stdafx.h"
#include "Level.h"
#include "Enity\EntitySystem.h"

#include <Threading\JobSystem2.h>

#include <I3DEngine.h>
#include <IWorld3D.h>
#include <IFileSys.h>
#include <IFrameData.h>

#include <UserCmdMan.h>

using namespace sound::Literals;

X_NAMESPACE_BEGIN(game)

Level::Level(physics::IScene* pScene, engine::IWorld3D* p3DWorld,
    entity::EnititySystem& entSys, core::MemoryArenaBase* arena) :
    arena_(arena),
    stringTable_(arena),
    entSys_(entSys),
    loaded_(false),
    entDescLoadedSignal_(false),
    p3DWorld_(p3DWorld),
    pScene_(pScene)
{
    pJobSys_ = X_ASSERT_NOT_NULL(gEnv->pJobSys);
    pFileSys_ = X_ASSERT_NOT_NULL(gEnv->pFileSys);
}

Level::~Level()
{
    clear();
}

void Level::update(core::FrameData& frame)
{
    X_UNUSED(frame);
}

void Level::beginLoad(const MapNameStr& name)
{
    path_ = assetDb::AssetType::ToString(assetDb::AssetType::LEVEL);
    path_.append('s', 1);
    path_.toLower();
    path_.append(assetDb::ASSET_NAME_SLASH, 1);
    path_.append(name.begin(), name.end());
    path_.setExtension(level::LVL_FILE_EXTENSION);

    clear();

    X_LOG0("Level", "Loading level: %s", name.c_str());

    core::IoRequestOpen open;
    open.callback.Bind<Level, &Level::IoRequestCallback>(this);
    open.mode = core::FileFlag::READ | core::FileFlag::SHARE;
    open.path = path_;

    pFileSys_->AddIoRequestToQue(open);

    // add a request for the ent desc.
    open.callback.Bind<Level, &Level::IoRequestCallbackEntDesc>(this);
    open.path.setExtension("json");

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

    entDescSize_ = 0;
    entDescData_.reset();

    entDescLoadedSignal_.clear();
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
    else {
        X_ASSERT_UNREACHABLE();
    }
}

void Level::IoRequestCallbackEntDesc(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
    core::XFileAsync* pFile, uint32_t bytesTransferred)
{
    core::IoRequest::Enum requestType = pRequest->getType();

    if (requestType == core::IoRequest::OPEN) {
        if (!pFile) {
            X_WARNING("Level", "Failed to open level ent desc");
            entDescLoadedSignal_.raise();
            return;
        }

        auto size = safe_static_cast<size_t>(pFile->fileSize());
        X_ASSERT(size > 0, "Handle size been zero")();
        X_ASSERT(arena_->isThreadSafe(), "Arena needs to be thread safe")(arena_->isThreadSafe());

        entDescSize_ = size;
        entDescData_ = core::makeUnique<char[]>(arena_, size, 64);

        core::IoRequestRead read;
        read.callback.Bind<Level, &Level::IoRequestCallbackEntDesc>(this);
        read.pFile = pFile;
        read.dataSize = safe_static_cast<uint32_t>(size);
        read.offset = 0;
        read.pBuf = entDescData_.ptr();

        fileSys.AddIoRequestToQue(read);
    }
    else if (requestType == core::IoRequest::READ) {

        if (!bytesTransferred) {
            entDescData_.reset();
        }

        // we are done :D
        entDescLoadedSignal_.raise();

        core::IoRequestClose req;
        req.pFile = pFile;
        fileSys.AddIoRequestToQue(req);
    }
    else {
        X_ASSERT_UNREACHABLE();
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

    X_ASSERT(arena_->isThreadSafe(), "Arena needs to be thread safe")(arena_->isThreadSafe());

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

    //	core::Thread::sleep(5000);

    // activate the scene and 3d world.
    gEnv->p3DEngine->addWorldToActiveList(p3DWorld_);
    gEnv->pPhysics->addSceneToSim(pScene_);

    X_LOG0("Level", "Finished loading");
    loaded_ = true;
}

bool Level::processHeader(void)
{
    // is this header valid m'lady?
    if (!fileHdr_.isValid()) {
        // this header is not valid :Z
        if (fileHdr_.fourCC == level::LVL_FOURCC_INVALID) {
            X_ERROR("Level", "\"%s\" file is corrupt, please re-compile.", path_.fileName());
            return false;
        }

        X_ERROR("Level", "\"%s\" is not a valid level", path_.fileName());
        return false;
    }

    if (fileHdr_.version != level::LVL_VERSION) {
        X_ERROR("Level", "\"%s\" has a invalid version. provided: %i required: %i",
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
#if 0 // don't think this is used currently.
    {
        core::XFileFixedBuf file = fileHdr_.FileBufForNode(levelData_.ptr(), level::FileNodes::ENTITIES);

        entSys_.loadEntites(
            reinterpret_cast<const char*>(file.getBufferStart()),
            reinterpret_cast<const char*>(file.getBufferEnd()));
    }
#endif

    // wait for the ent desc to finish loading.
    entDescLoadedSignal_.wait();

    if (entDescData_) {

        const char* pBegin = entDescData_.ptr();
        const char* pEnd = pBegin + entDescSize_;

        if (!entSys_.loadEntites(pBegin, pEnd)) {

            return false;
        }

        entDescSize_ = 0;
        entDescData_.reset();
    }

    entSys_.postLoad();
    return true;
}

// -------------------------------------

World::World(GameVars& vars, net::SessionInfo& sessionInfo, physics::IPhysics* pPhys,
    game::weapon::WeaponDefManager& weaponDefs, Multiplayer* pMultiplayer, core::MemoryArenaBase* arena) :
    arena_(arena),
    pPhys_(pPhys),
    pScene_(nullptr),
    p3DWorld_(nullptr),
    ents_(vars, sessionInfo, weaponDefs, pMultiplayer, arena),
    level_(arena)
{
}

World::~World()
{
    level_.reset();

    ents_.shutdown();

    if (pScene_) {
        gEnv->pSound->unRegisterAll();
        gEnv->pSound->setPhysicsScene(nullptr);

        pPhys_->releaseScene(pScene_);
    }

    if (p3DWorld_) {
        gEnv->p3DEngine->release3DWorld(p3DWorld_);
    }
}

bool World::loadMap(const MapNameStr& name)
{
    X_LOG0("Game", "Loading map: \"%s\"", name.c_str());

    if (!createPhysicsScene(pPhys_)) {
        return false;
    }

    gEnv->pSound->setPhysicsScene(pScene_);
    
    p3DWorld_ = gEnv->p3DEngine->create3DWorld(pScene_);

    if (!ents_.init(pPhys_, pScene_, p3DWorld_)) {
        return false;
    }

    level_ = core::makeUnique<Level>(arena_, pScene_, p3DWorld_, ents_, arena_);
    level_->beginLoad(name);

    return true;
}

bool World::hasLoaded(void) const
{
    X_ASSERT(level_, "Level not valid")();

    return level_->isLoaded();
}

void World::runUserCmdForPlayer(core::TimeVal dt, const net::UserCmd& cmd, int32_t playerIdx)
{
    X_ASSERT(playerIdx < net::MAX_PLAYERS, "Invalid player idx")(playerIdx, net::MAX_PLAYERS);

    ents_.runUserCmdForPlayer(dt, cmd, static_cast<entity::EntityId>(playerIdx));
}

void World::update(core::FrameData& frame, net::UserCmdMan& userCmdMan, entity::EntityId localPlayerId)
{
    X_UNUSED(userCmdMan);

    X_ASSERT(level_ && level_->isLoaded(), "Level not valid")();

    ents_.update(frame, userCmdMan, localPlayerId);

    level_->update(frame);
}

void World::createSnapShot(net::SnapShot& snap)
{
    ents_.createSnapShot(snap);
}

void World::applySnapShot(const UserNetMappings& unm, const net::SnapShot& snap)
{
    ents_.applySnapShot(unm, snap);
}


void World::spawnPlayer(const UserNetMappings& unm, int32_t playerIdx, bool local)
{
    X_ASSERT(playerIdx < net::MAX_PLAYERS, "Invalide player id")(playerIdx, net::MAX_PLAYERS);
    X_ASSERT_NOT_NULL(pScene_);

    // we want to take the give ent and set it up as a player.
    // the ent may not have all the required components yet.
    // we also want to support spawning diffrent types of players with various definitions.
    // maybe i should write some system for defining what each enity type has.
    // for now can be just in code, but setup in way that can be data driven.
    // which ia suspect should be handled by the enitiy system.
    // so we allmost what a make ent type helper, that we can just call and it makes the ent a player.
    // then here we can do shit with the player.
    entity::EntityId id = static_cast<entity::EntityId>(playerIdx);
    auto pos = Vec3f(-80, -50.f + (playerIdx * 50.f), 10);

    ents_.spawnPlayer(unm, id, pos, local);
}

void World::removePlayer(int32_t playerIdx)
{
    X_ASSERT(playerIdx < net::MAX_PLAYERS, "Invalide player id")(playerIdx, net::MAX_PLAYERS);
    X_ASSERT_NOT_NULL(pScene_);

    entity::EntityId id = static_cast<entity::EntityId>(playerIdx);

    ents_.removePlayer(id);
}

bool World::createPhysicsScene(physics::IPhysics* pPhys)
{
    if (pScene_) {
        pPhys->releaseScene(pScene_);
    }

    // lets make a scene.
    physics::SceneDesc sceneDesc;
    core::zero_object(sceneDesc.sceneLimitHint); // zero = no limit
    sceneDesc.gravity = Vec3f(0.f, 0.f, -98.0f);
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
