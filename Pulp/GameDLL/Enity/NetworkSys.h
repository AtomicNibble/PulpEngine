#pragma once

#include "EnityComponents.h"

#include <INetwork.h>

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
        using EntityIdMapArr = std::array< EntityId, net::MAX_SYNCED_ENTS>;


    public:
        bool init(void);

        void buildSnapShot(EnitiyRegister& reg, net::SnapShot& snap, physics::IScene* pScene);
        void applySnapShot(EnitiyRegister& reg, const net::SnapShot& snap, physics::IScene* pScene, engine::IWorld3D* p3DWorld);

    private:
        EntityIdMapArr entIdMap_;
    };

} // namespace entity

X_NAMESPACE_END