#include "stdafx.h"
#include "NetworkSys.h"

#include <IFrameData.h>

X_NAMESPACE_BEGIN(game)

namespace entity
{
    bool NetworkSystem::init(void)
    {

        return true;
    }

    void NetworkSystem::buildSnapShot(core::FrameTimeData& timeInfo, EnitiyRegister& reg, net::SnapShot& snap)
    {
        X_UNUSED(timeInfo, reg, snap);

        auto view = reg.view<NetworkSync>();
        for (auto entity : view)
        {
            auto& netSync = reg.get<NetworkSync>(entity);

            X_UNUSED(netSync);
            // you want to be synced, fuck you!



        }
    }


} // namespace entity

X_NAMESPACE_END