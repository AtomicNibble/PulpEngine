#pragma once

#include <IAnimManager.h>

X_NAMESPACE_DECLARE(core,
                    struct FrameData)

X_NAMESPACE_BEGIN(game)

class GameVars;

namespace entity
{
    class WeaponSystem
    {
    public:
        WeaponSystem(GameVars& vars);

        bool init(ECS& reg, physics::IScene* pPhysScene, anim::IAnimManager* pAnimManager);
        void update(core::FrameData& frame, ECS& reg);

    private:
        void beginRaise(core::TimeVal curTime, Weapon& wpn, Animator& animator);
        void beginLower(core::TimeVal curTime, Weapon& wpn, Animator& animator);
        void beginIdle(core::TimeVal curTime, Weapon& wpn, Animator& animator);
        void beginAttack(core::TimeVal curTime, Weapon& wpn, Animator& animator, core::FrameData& frame);
        bool beginReload(core::TimeVal curTime, Weapon& wpn, Animator& animator);

        void trainsitionToState(Weapon& wpn, Animator& animator, weapon::AnimSlot::Enum anim, weapon::State::Enum newState,
            core::TimeVal curTime, core::TimeVal transTime);

        void doDamage(EntityId ent, Weapon& wpn, float distance);
        void serverDoDamage(EntityId ent, Weapon& wpn, int32_t dmg);

    private:
        GameVars& vars_;

        ECS* pReg_;

        physics::IScene* pPhysScene_;
        anim::IAnimManager* pAnimManager_;
    };

} // namespace entity

X_NAMESPACE_END
