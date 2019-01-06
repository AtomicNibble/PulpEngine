#include "stdafx.h"
#include "NetworkSys.h"

X_NAMESPACE_BEGIN(game)

namespace entity
{
    bool NetworkSystem::init(void)
    {
        return true;
    }

   
    void NetworkSystem::clientUpdate(ECS& reg, float frac)
    {
        X_ASSERT(frac >= 0.f && frac <= 1.f, "Frac out of range")(frac);

        auto view = reg.view<NetworkSync>();
        for (auto entity : view) 
        {
            auto& ns = reg.get<NetworkSync>(entity);

            ns.current.pos = ns.prev.pos.lerp(frac, ns.next.pos);
            ns.current.quat = ns.prev.quat.slerp(frac, ns.next.quat);
        }
    }


} // namespace entity

X_NAMESPACE_END