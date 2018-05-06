#pragma once

#include "EnityComponents.h"


X_NAMESPACE_DECLARE(core,
    struct FrameTimeData)

X_NAMESPACE_DECLARE(net,
    class SnapShot)

X_NAMESPACE_BEGIN(game)

namespace entity
{
    class NetworkSystem
    {
    public:
        bool init(void);
        void buildSnapShot(core::FrameTimeData& timeInfo, EnitiyRegister& reg, net::SnapShot& snap);

    private:
  
    };

} // namespace entity

X_NAMESPACE_END