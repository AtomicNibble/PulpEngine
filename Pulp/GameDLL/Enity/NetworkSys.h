#pragma once

#include "EnityComponents.h"

X_NAMESPACE_DECLARE(engine,
    struct IWorld3D)

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
        void applySnapShot(core::FrameTimeData& timeInfo, EnitiyRegister& reg, const net::SnapShot* pSnap,
            physics::IScene* pScene, engine::IWorld3D* p3DWorld);

    private:
  
    };

} // namespace entity

X_NAMESPACE_END