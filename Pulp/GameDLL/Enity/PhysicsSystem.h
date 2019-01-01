#pragma once

#include "EntityComponents.h"

X_NAMESPACE_DECLARE(core,
                    struct FrameTimeData;
                    struct ICVar;)

X_NAMESPACE_DECLARE(engine,
                    struct IWorld3D;)

X_NAMESPACE_BEGIN(game)

namespace entity
{
    class PhysicsSystem
    {
    public:
        PhysicsSystem();
        ~PhysicsSystem();

        bool init(ECS& reg, physics::IScene* pPhysScene);

        void update(core::FrameData& frame, ECS& reg);
        bool createColliders(ECS& reg, physics::IPhysics* pPhysics, physics::IScene* pPhysScene);

        void onMsg(ECS& reg, const MsgMove& msg);

    private:
        physics::IScene* pPhysScene_;
    };

} // namespace entity

X_NAMESPACE_END