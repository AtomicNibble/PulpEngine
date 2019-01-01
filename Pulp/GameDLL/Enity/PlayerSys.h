#pragma once

#include "Vars\PlayerVars.h"

X_NAMESPACE_DECLARE(core,
    struct FrameTimeData)

X_NAMESPACE_DECLARE(engine,
    struct IWorld3D)

X_NAMESPACE_DECLARE(model,
    struct IModelManager)

X_NAMESPACE_BEGIN(game)

namespace weapon
{
    class WeaponDefManager;
} // namespace weapon

namespace entity
{
    class PlayerSystem
    {
    public:
        PlayerSystem(PlayerVars& playerVars);

        bool init(physics::IScene* pPhysScene);

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

        physics::IScene* pPhysScene_;
    };

} // namespace entity

X_NAMESPACE_END
