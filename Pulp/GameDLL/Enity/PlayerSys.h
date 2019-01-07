#pragma once

X_NAMESPACE_DECLARE(core,
    struct FrameTimeData)

X_NAMESPACE_DECLARE(engine,
    struct IWorld3D)

X_NAMESPACE_DECLARE(model,
    struct IModelManager)

X_NAMESPACE_BEGIN(game)

class PlayerVars;

namespace weapon
{
    class WeaponDefManager;
} // namespace weapon

namespace entity
{
    class PlayerSystem
    {
    public:
        PlayerSystem(PlayerVars& playerVars, const net::SessionInfo& sessionInfo);

        bool init(ECS& reg, physics::IScene* pPhysScene, engine::IWorld3D* p3DWorld);

        void onMsg(ECS& reg, const MsgMove& msg);

        void update(core::FrameTimeData& timeInfo, ECS& reg);

        void runUserCmdForPlayer(core::TimeVal dt, ECS& reg,
            game::weapon::WeaponDefManager& weaponDefs, model::IModelManager* pModelManager, engine::IWorld3D* p3DWorld,
            const net::UserCmd& userCmd, EntityId playerId);

    private:
        void updateViewBob(core::TimeVal dt, Player& player);
        void updateEye(Player& player);
        void calculateFirstPersonView(TransForm& trans, Player& player);

    private:
        PlayerVars& vars_;
        const net::SessionInfo& sessionInfo_;

        physics::IScene* pPhysScene_;
        engine::IWorld3D* p3DWorld_;

    };

} // namespace entity

X_NAMESPACE_END
