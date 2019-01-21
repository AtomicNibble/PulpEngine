#pragma once

#include <IInput.h>
#include <IModelManager.h>

#include "EntityComponents.h"

#include "CameraSys.h"
#include "PlayerSys.h"
#include "SoundSys.h"
#include "PhysicsSystem.h"
#include "AnimatedSys.h"
#include "WeaponSystem.h"
#include "EmitterSys.h"
#include "MeshRendererSys.h"
#include "HealthSys.h"
#include "NetworkSys.h"
#include "LightSys.h"

#include "DataTranslator.h"


X_NAMESPACE_DECLARE(net,
                    struct ISession);

X_NAMESPACE_DECLARE(core,
                    struct FrameTimeData)

X_NAMESPACE_DECLARE(engine,
                    struct IWorld3D;
                    namespace fx {
                        struct IEffectManager;
                    })

X_NAMESPACE_DECLARE(net,
                    class SnapShot;
                    class UserCmdMan;)

X_NAMESPACE_BEGIN(game)


class Multiplayer;
class GameVars;
struct UserNetMappings;
struct NetInterpolationState;

namespace weapon
{
    class WeaponDefManager;
}

namespace entity
{
    static const EnitiyRegister::entity_type INVALID_ID = EnitiyRegister::INVALID_ID;

    class EnititySystem
    {
        using EntityIdMapArr = std::array< EntityId, MAX_ENTS>;
         
        typedef core::MemoryArena<
            core::MallocFreeAllocator,
            core::SingleThreadPolicy,
#if X_DEBUG
            core::SimpleBoundsChecking,
            core::SimpleMemoryTracking,
            core::SimpleMemoryTagging
#else
            core::NoBoundsChecking,
            core::NoMemoryTracking,
            core::NoMemoryTagging
#endif // !X_DEBUG
        >
            ECSArena;

    public:
        typedef EnitiyRegister::entity_type EntityId;

    public:
        EnititySystem(GameVars& vars, net::SessionInfo& sessionInfo, weapon::WeaponDefManager& weaponDefs,
            Multiplayer* pMultiplayer, core::MemoryArenaBase* arena);

        bool init(physics::IPhysics* pPhysics, physics::IScene* pPhysScene, engine::IWorld3D* p3DWorld);
        void shutdown(void);
        void runUserCmdForPlayer(core::TimeVal dt, const net::UserCmd& cmd, EntityId playerId);
        void update(core::FrameData& frame, net::UserCmdMan& userCmdMan, const NetInterpolationState& netInterpolState, EntityId localPlayerId);

        void createSnapShot(net::SnapShot& snap);
        void applySnapShot(const UserNetMappings& unm, const net::SnapShot& snap);

        void destroy(Mesh& comp);
        void destroy(MeshRenderer& comp);
        void destroy(MeshCollider& comp);
        void destroy(DynamicObject& comp);
        void destroy(Weapon& comp);
        void destroy(Emitter& comp);
        void destroy(Light& comp);
        void destroy(Animator& comp);
        void destroy(CharacterController& comp);
        void destroy(Player& comp);

        void onMsg(ECS& reg, const MsgPlayerDied& msg);


        EntityId createEnt(void);
        void destroyEnt(EntityId id);
        void removePlayer(EntityId id);

        EntityId createWeapon(EntityId playerId);

        void spawnPlayer(const UserNetMappings& unm, int32_t clientIdx, const Vec3f& pos, bool local);
        bool addController(EntityId id);

        bool loadEntites(const char* pJsonBegin, const char* pJsonEnd);

        bool postLoad(void);

    private:
        bool createTranslatours(void);

        bool parseEntites(const char* pJsonBegin, const char* pJsonEnd);

        template<typename CompnentT>
        static bool parseComponent(DataTranslator<CompnentT>& translator, CompnentT& comp, const core::json::Value& compDesc);

        static bool parseColor(const core::json::Value& value, Color8u& col);
        static bool parseVec(const core::json::Value& value, Vec3f& vec);

        bool parseEntDesc(core::json::Value& val);

    private:
        core::MemoryArenaBase* arena_;
        // before reg_
        ECSArena::AllocationPolicy ecsAllocator_;
        ECSArena ecsArena_;

        ECS reg_;
        GameVars& vars_;
        net::SessionInfo& sessionInfo_;
        game::weapon::WeaponDefManager& weaponDefs_;
        Multiplayer* pMultiplayer_;

        physics::IPhysics* pPhysics_;
        physics::IScene* pPhysScene_;
        engine::IWorld3D* p3DWorld_;
        model::IModelManager* pModelManager_;
        engine::fx::IEffectManager* pEffectManager_;

        PlayerSystem playerSys_;
        CameraSystem cameraSys_;
        SoundSystem soundSys_;
        PhysicsSystem physSys_;
        AnimatedSystem animatedSys_;
        WeaponSystem weaponSys_;
        EmitterSys emitterSys_;
        MeshRendererSys meshRendererSys_;
        HealthSystem healthSys_;
        NetworkSystem networkSys_;
        LightSystem lightSys_;

        EntityId endOfmapEnts_;
        EntityIdMapArr entIdMap_;

    private:
        DataTranslator<Health> dtHealth_;
        DataTranslator<SoundObject> dtSoundObj_;
        DataTranslator<Rotator> dtRotator_;
        DataTranslator<Mover> dtMover_;
        DataTranslator<Emitter> dtEmitter_;
        DataTranslator<DynamicObject> dtDynamicObject_;
    };

} // namespace entity

X_NAMESPACE_END
