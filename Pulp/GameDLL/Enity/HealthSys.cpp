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

        if (h.hp <= 0) {
            return;
        }

        h.hp -= msg.damage;

        // cap it?
        if (h.hp < -999) {
            h.hp = -999;
        }

        X_LOG0("Health", "Ent %" PRIi32 " took Dmg: %" PRIi32 " Ent health %" PRIi32, static_cast<int32_t>(msg.id), msg.damage, h.hp);

        // just died?
        if (h.hp <= 0) {
            X_LOG0("Health", "Ent %" PRIi32 " died", static_cast<int32_t>(msg.id));

            reg.markDestroy(msg.id);
        }
    }

} // namespace entity

X_NAMESPACE_END
