#pragma once

X_NAMESPACE_DECLARE(core,
                    struct FrameTimeData;)

X_NAMESPACE_DECLARE(engine,
                    struct IWorld3D;)

X_NAMESPACE_BEGIN(game)

namespace entity
{
    class AnimatedSystem
    {
    public:
        AnimatedSystem();

        bool init(void);
        void update(core::FrameTimeData& time, ECS& reg, engine::IWorld3D* p3DWorld);

    private:
    };

} // namespace entity

X_NAMESPACE_END