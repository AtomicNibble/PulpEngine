#pragma once

X_NAMESPACE_DECLARE(core,
                    struct FrameTimeData;)

X_NAMESPACE_BEGIN(game)

namespace entity
{
    class SoundSystem
    {
    public:
        SoundSystem();

        bool init(ECS& reg);
        void update(core::FrameData& frame, ECS& reg);

        void onMsg(ECS& reg, const MsgMove& msg);


    private:
        sound::ISound* pSound_;
    };

} // namespace entity

X_NAMESPACE_END