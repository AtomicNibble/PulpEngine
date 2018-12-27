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

class GameVars;
struct UserNetMappings;

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

    public:
        typedef EnitiyRegister::entity_type EntityId;

    public:
        EnititySystem(GameVars& vars, game::weapon::WeaponDefManager& weaponDefs, core::MemoryArenaBase* arena);

        bool init(physics::IPhysics* pPhysics, physics::IScene* pPhysScene, engine::IWorld3D* p3DWorld);
        void shutdown(void);
        void runUserCmdForPlayer(core::TimeVal dt, const net::UserCmd& cmd, EntityId playerId);
        void update(core::FrameData& frame, net::UserCmdMan& userCmdMan, EntityId localPlayerId);

        void createSnapShot(net::SnapShot& snap);
        void applySnapShot(const UserNetMappings& unm, const net::SnapShot& snap);


        EntityId createEnt(void);
        void destroyEnt(EntityId id);
        EntityId createWeapon(EntityId playerId);

        void makePlayer(EntityId id, const Vec3f& pos, bool local);
        void removePlayer(EntityId id);
        bool addController(EntityId id);

        bool loadEntites(const char* pJsonBegin, const char* pJsonEnd);

        bool postLoad(void);

        X_INLINE const EnitiyRegister& getRegister(void) const;
        X_INLINE EnitiyRegister& getRegister(void);

    private:
        bool createTranslatours(void);

        bool parseEntites(const char* pJsonBegin, const char* pJsonEnd);

        template<typename CompnentT>
        static bool parseComponent(DataTranslator<CompnentT>& translator, CompnentT& comp, const core::json::Value& compDesc);

        bool parseEntDesc(core::json::Value& val);

    private:
        core::MemoryArenaBase* arena_;
        EnitiyRegister reg_;
        GameVars& vars_;
        game::weapon::WeaponDefManager& weaponDefs_;

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

        EntityId endOfmapEnts_;
        EntityIdMapArr entIdMap_;

    private:
        DataTranslator<Health> dtHealth_;
        DataTranslator<Mesh> dtMesh_;
        DataTranslator<SoundObject> dtSoundObj_;
        DataTranslator<Rotator> dtRotator_;
        DataTranslator<Mover> dtMover_;
        DataTranslator<Emitter> dtEmitter_;
        DataTranslator<DynamicObject> dtDynamicObject_;
    };

    X_INLINE const EnitiyRegister& EnititySystem::getRegister(void) const
    {
        return reg_;
    }

    X_INLINE EnitiyRegister& EnititySystem::getRegister(void)
    {
        return reg_;
    }

} // namespace entity

X_NAMESPACE_END