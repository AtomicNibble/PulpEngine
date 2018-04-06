#pragma once

#include "EnityComponents.h"

#include <IInput.h>

X_NAMESPACE_DECLARE(core,
                    struct FrameTimeData;)

X_NAMESPACE_BEGIN(game)

namespace entity
{
    class InputSystem : public input::IInputEventListner
    {
    public:
        bool init(void);
        void update(core::FrameTimeData& timeInfo, EnitiyRegister& reg, physics::IScene* pPhysScene);

    private:
        void processInput(core::FrameTimeData& timeInfo);

    private:
        // IInputEventListner
        bool OnInputEvent(const input::InputEvent& event) X_FINAL;
        bool OnInputEventChar(const input::InputEvent& event) X_FINAL;
        // ~IInputEventListner

    private:
        input::InputEventBuffer inputEvents_;
    };

} // namespace entity

X_NAMESPACE_END