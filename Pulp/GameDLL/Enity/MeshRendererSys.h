#pragma once

X_NAMESPACE_DECLARE(core,
    struct FrameTimeData)

X_NAMESPACE_DECLARE(engine,
    struct IWorld3D)

X_NAMESPACE_BEGIN(game)

namespace entity
{
    class MeshRendererSys
    {
    public:
        MeshRendererSys();

        bool init(ECS& reg, engine::IWorld3D* p3DWorld);
        void update(core::FrameData& frame, ECS& reg);

        void onMsg(ECS& reg, const MsgMove& msg);

    private:
        engine::IWorld3D* p3DWorld_;
    };

} // namespace entity

X_NAMESPACE_END