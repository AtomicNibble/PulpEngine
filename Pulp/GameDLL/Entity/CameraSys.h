#pragma once

#include "EntityComponents.h"

X_NAMESPACE_DECLARE(core,
                    struct FrameTimeData;
                    struct ICVar;)

X_NAMESPACE_BEGIN(game)

class GameVars;

namespace entity
{
    class CameraSystem
    {
    public:
        CameraSystem(GameVars& vars);
        ~CameraSystem();

        bool init(void);
        void update(core::FrameData& frame, EnitiyRegister& reg, physics::IScene* pPhysScene);

        void setActiveEnt(EntityId entId);

    private:
        void OnFovChanged(core::ICVar* pVar);

    private:
        GameVars& vars_;
        EntityId activeEnt_;

        bool setCamPos_;
        bool setCamAngle_;

        Vec3f cameraPos_;
        Vec3f cameraAngle_;
        Vec3f cameraAngleDeg_;

        XCamera cam_;
    };

} // namespace entity

X_NAMESPACE_END