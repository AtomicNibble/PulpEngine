#include "stdafx.h"
#include "HealthSys.h"

X_NAMESPACE_BEGIN(game)

namespace entity
{
    HealthSystem::HealthSystem()
    {

    }

    bool HealthSystem::init(ECS& reg)
    {
        reg.registerHandler<HealthSystem, MsgDamage>(this);
        return true;
    }

    void HealthSystem::update(core::FrameTimeData& frame, ECS& reg)
    {
        X_UNUSED(frame, reg);
    }

    void HealthSystem::onMsg(ECS& reg, const MsgDamage& msg)
    {
        if (!reg.has<Health>(msg.id)) {
            return;
        }

        auto& h = reg.get<Health>(msg.id);

        h.hp -= msg.damage;
    }

} // namespace entity

X_NAMESPACE_END
