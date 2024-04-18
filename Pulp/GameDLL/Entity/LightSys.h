#pragma once

X_NAMESPACE_DECLARE(core,
    struct FrameData)

X_NAMESPACE_DECLARE(engine,
    struct IWorld3D)

X_NAMESPACE_BEGIN(game)

namespace entity
{
    class LightSystem
    {
    public:
        LightSystem();

        bool init(ECS& reg, engine::IWorld3D* p3DWorld);
        void update(const core::FrameData& frame, ECS& reg);

        void onMsg(ECS& reg, const MsgMove& msg);

    private:
        engine::IWorld3D* p3DWorld_;
    };

} // namespace entity

X_NAMESPACE_END