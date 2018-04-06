#pragma once

#include "Vars\PlayerVars.h"

X_NAMESPACE_DECLARE(core,
                    struct FrameTimeData;)

X_NAMESPACE_DECLARE(engine,
                    struct IWorld3D;)

X_NAMESPACE_BEGIN(game)

namespace entity
{
    class PlayerSystem
    {
    public:
        PlayerSystem(PlayerVars& playerVars);

        bool init(physics::IScene* pPhysScene);

        void runUserCmdForPlayer(core::FrameTimeData& timeInfo, EnitiyRegister& reg, engine::IWorld3D* p3DWorld,
            const UserCmd& userCmd, EntityId playerId);

    private:
        void updateViewBob(core::FrameTimeData& timeInfo, Player& player);
        void updateEye(Player& player);
        void calculateFirstPersonView(TransForm& trans, Player& player);

    private:
        PlayerVars& vars_;

        physics::IScene* pPhysScene_;
    };

} // namespace entity

X_NAMESPACE_END