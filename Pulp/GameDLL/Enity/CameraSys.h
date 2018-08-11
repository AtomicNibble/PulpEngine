#pragma once

#include "EnityComponents.h"

X_NAMESPACE_DECLARE(core,
                    struct FrameTimeData;
                    struct ICVar;)

X_NAMESPACE_BEGIN(game)

namespace entity
{
    class CameraSystem
    {
    public:
        CameraSystem();
        ~CameraSystem();

        bool init(void);
        void update(core::FrameData& frame, EnitiyRegister& reg, physics::IScene* pPhysScene);

        void setActiveEnt(EntityId entId);

    private:
        void OnFovChanged(core::ICVar* pVar);

    private:
        EntityId activeEnt_;

        bool setCamPos_;
        bool setCamAngle_;

        Vec3f cameraPos_;
        Vec3f cameraAngle_;
        Vec3f cameraAngleDeg_;

        XCamera cam_;

        core::ICVar* pFovVar_;
    };

} // namespace entity

X_NAMESPACE_END