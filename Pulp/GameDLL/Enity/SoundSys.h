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

        bool init(void);
        void update(core::FrameData& frame, EnitiyRegister& reg);

    private:
        sound::ISound* pSound_;
    };

} // namespace entity

X_NAMESPACE_END