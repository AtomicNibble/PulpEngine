#pragma once

#include <IInput.h>
#include "UserCmd.h"

X_NAMESPACE_BEGIN(game)

class InputVars;

X_DECLARE_ENUM(UserButton)
(
    NONE,

    ATTACK,
    ZOOM,
    RUN,
    USE,
    RELOAD,

    MOVE_BACK,
    MOVE_FORWARD,
    MOVE_LEFT,
    MOVE_RIGHT,
    MOVE_UP,
    MOVE_DOWN,

    WEAP0,
    WEAP1,
    WEAP2,
    WEAP3,
    WEAP4,
    WEAP5,
    WEAP6,
    WEAP7,
    WEAP8,
    WEAP9,
    WEAP_NEXT,
    WEAP_PREV
    );

class ButtonState
{
public:
    ButtonState();

    void clear(void);
    void setKeyState(int32_t keystate, bool toggle);

    bool isActive(void) const;

private:
    int32_t active_;
    bool held_;
};


class UserCmdGen : public ICoreEventListener
{
public:
    UserCmdGen(const InputVars& vars);

    bool init(void);
    void shutdown(void);

    void clear(void);
    void clearAngles(void);

    bool onInputEvent(const input::InputEvent& event);
    void buildUserCmd(bool blockInput);

    net::UserCmd& getCurrentUsercmd(void);
    const net::UserCmd& getCurrentUsercmd(void) const;

private:
    void resetCmd(void);
    void mouseMove(void);
    void keyMove(void);
    void setButtonFlags(void);
    void processInput(void);


    void OnCoreEvent(const CoreEventData& ed) X_FINAL;

private:
    static UserButton::Enum getUserButton(input::KeyId::Enum key);

    void setButtonState(input::KeyId::Enum key, bool down);
    int32_t buttonState(UserButton::Enum but) const;

private:
    const InputVars& vars_;
    net::UserCmd cmd_;

    ButtonState toggledCrouch_;
    ButtonState toggledRun_;
    ButtonState toggledZoom_;

    Vec2f mouseDelta_;
    Vec3f viewAngles_;

    input::InputEventBuffer inputEvents_;

    std::array<int32_t, UserButton::ENUM_COUNT> buttonStates_;
    std::array<bool, input::KeyId::ENUM_COUNT> keyState_;
};

X_NAMESPACE_END