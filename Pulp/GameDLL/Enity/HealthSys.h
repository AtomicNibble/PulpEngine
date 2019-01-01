#pragma once

X_NAMESPACE_DECLARE(core,
                    struct FrameTimeData;)


X_NAMESPACE_BEGIN(game)

namespace entity
{
    class HealthSystem
    {
    public:
        HealthSystem();

        bool init(ECS& reg);
        void update(core::FrameTimeData& time, ECS& reg);

        void onMsg(ECS& reg, const MsgDamage& msg);

    private:
    };

} // namespace entity

X_NAMESPACE_END