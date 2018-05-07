#pragma once

#include <Util\UniquePointer.h>
#include <String\GrowingStringTable.h>

#include <Ilevel.h>

#include "Enity\EnitiySystem.h"

X_NAMESPACE_DECLARE(core,
                    namespace V2 {
                        struct Job;
                        class JobSystem;
                    }

                    struct XFileAsync;

                    struct FrameData;)

X_NAMESPACE_DECLARE(engine,
                    struct IWorld3D;)

X_NAMESPACE_BEGIN(game)

class GameVars;
class UserCmdMan;

namespace entity
{
    class EnititySystem;

} // namespace entity

class Level
{
public:
    Level(physics::IScene* pScene, engine::IWorld3D* p3DWorld, entity::EnititySystem& entSys, core::MemoryArenaBase* arena);
    ~Level();

    void update(core::FrameData& frame);

    void beginLoad(const core::string& name);
    void clear(void);

    X_INLINE bool isLoaded(void) const;

private:
    void IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
        core::XFileAsync* pFile, uint32_t bytesTransferred);

    void processHeader_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);
    void processData_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);
    bool processHeader(void);
    bool processData(void);
    bool processEnts(void);

private:
    core::MemoryArenaBase* arena_;
    core::V2::JobSystem* pJobSys_;
    core::IFileSys* pFileSys_;
    entity::EnititySystem& entSys_;

    bool loaded_;
    bool _pad[3];

    core::Path<char> path_;
    level::FileHeader fileHdr_;
    core::UniquePointer<uint8_t[]> levelData_;

    engine::IWorld3D* p3DWorld_;
    physics::IScene* pScene_;

    level::StringTable stringTable_;
};

X_INLINE bool Level::isLoaded(void) const
{
    return loaded_;
}

class World
{
public:
    World(GameVars& vars, physics::IPhysics* pPhys, UserCmdMan& userCmdMan,
        game::weapon::WeaponDefManager& weaponDefs, core::MemoryArenaBase* arena);
    ~World();

    bool loadMap(const core::string& name);
    bool hasLoaded(void) const;

    void update(core::FrameData& frame, UserCmdMan& userCmdMan);

    void createSnapShot(core::FrameData& frame, net::SnapShot& snap);
    void applySnapShot(core::FrameData& frame, const net::SnapShot* pSnap);

    void spawnPlayer(entity::EntityId id);

private:
    bool createPhysicsScene(physics::IPhysics* pPhys);

private:
    core::MemoryArenaBase* arena_;
    physics::IPhysics* pPhys_;
    physics::IScene* pScene_;

    entity::EnititySystem ents_;
    core::UniquePointer<Level> level_;

    UserCmdMan& userCmdMan_;
};

X_NAMESPACE_END