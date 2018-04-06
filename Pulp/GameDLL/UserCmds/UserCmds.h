#pragma once

#include <IInput.h>
#include "UserCmd.h"

X_NAMESPACE_BEGIN(game)

X_DECLARE_ENUM(UserButton)
(
    NONE,

    ATTACK,
    ZOOM,
    SPEED,
    USE,
    RELOAD,

    MOVE_BACK,
    MOVE_FORWARD,
    MOVE_LEFT,
    MOVE_RIGHT,
    MOVE_UP,
    MOVE_DOWN);

class UserCmdGen : public input::IInputEventListner
{
public:
    UserCmdGen();

    bool init(void);
    void shutdown(void);
    void clear(void);

    void buildUserCmd(void);

    UserCmd& getCurrentUsercmd(void);
    const UserCmd& getCurrentUsercmd(void) const;

private:
    void resetCmd(void);
    void mouseMove(void);
    void keyMove(void);
    void setButtonFlags(void);
    void processInput(void);

    // IInputEventListner
    bool OnInputEvent(const input::InputEvent& event) X_FINAL;
    bool OnInputEventChar(const input::InputEvent& event) X_FINAL;
    // ~IInputEventListner

private:
    void setButtonState(input::KeyId::Enum key, bool down);
    int32_t buttonState(UserButton::Enum but) const;

private:
    UserCmd cmd_;

    Vec2f mouseDelta_;
    Vec3f viewAngles_;

    input::InputEventBuffer inputEvents_;

    std::array<int32_t, UserButton::ENUM_COUNT> buttonStates_;
    std::array<bool, input::KeyId::ENUM_COUNT> keyState_;
};

X_NAMESPACE_END