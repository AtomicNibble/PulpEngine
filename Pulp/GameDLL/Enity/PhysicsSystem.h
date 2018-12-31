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

        bool init(void);

        void update(core::FrameData& frame, EnitiyRegister& reg, physics::IScene* pPhysScene, engine::IWorld3D* p3DWorld);

        bool createColliders(EnitiyRegister& reg, physics::IPhysics* pPhysics, physics::IScene* pPhysScene);

    private:
    };

} // namespace entity

X_NAMESPACE_END