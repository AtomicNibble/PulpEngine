#pragma once

#include <IInput.h>
#include <IModelManager.h>

#include "EnityComponents.h"

#include "CameraSys.h"
#include "PlayerSys.h"
#include "SoundSys.h"
#include "PhysicsSystem.h"
#include "AnimatedSys.h"
#include "WeaponSystem.h"
#include "NetworkSys.h"

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

X_NAMESPACE_BEGIN(game)

class GameVars;
class UserCmdMan;

namespace weapon
{
    class WeaponDefManager;
}

namespace entity
{
    static const EnitiyRegister::entity_type INVALID_ID = EnitiyRegister::INVALID_ID;

    class EnititySystem
    {
    public:
        typedef EnitiyRegister::entity_type EntityId;

    public:
        EnititySystem(GameVars& vars, game::weapon::WeaponDefManager& weaponDefs, core::MemoryArenaBase* arena);

        bool init(physics::IPhysics* pPhysics, physics::IScene* pPhysScene, engine::IWorld3D* p3DWorld);
        void runUserCmdForPlayer(core::FrameData& frame, const net::UserCmd& cmd, EntityId playerId);
        void update(core::FrameData& frame, UserCmdMan& userCmdMan, EntityId localPlayerId);

        void createSnapShot(core::FrameData& frame, net::SnapShot& snap);
        void applySnapShot(core::FrameData& frame, const net::SnapShot* pSnap);


        EntityId createEnt(void);
        void destroyEnt(EntityId id);
        EntityId createWeapon(EntityId playerId);

        void makePlayer(EntityId id);
        void removePlayer(EntityId id);
        bool addController(EntityId id);

        bool loadEntites(const char* pJsonBegin, const char* pJsonEnd);
        bool loadEntites2(const char* pJsonBegin, const char* pJsonEnd);

        bool postLoad(void);

        X_INLINE const EnitiyRegister& getRegister(void) const;
        X_INLINE EnitiyRegister& getRegister(void);

    private:
        bool createTranslatours(void);

        // Temp
        bool parseScriptOrigins(core::json::Value::Array val);

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
        NetworkSystem networkSys_;

    private:
        DataTranslator<Health> dtHealth_;
        DataTranslator<Mesh> dtMesh_;
        DataTranslator<SoundObject> dtSoundObj_;
        DataTranslator<Rotator> dtRotator_;
        DataTranslator<Mover> dtMover_;
        DataTranslator<Emitter> dtEmitter_;
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
