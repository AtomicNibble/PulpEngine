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

class UserCmdGen : public input::IInputEventListner, public ICoreEventListener
{
public:
    UserCmdGen();

    bool init(void);
    void shutdown(void);
    void clear(void);
    void clearAngles(void);

    void buildUserCmd(bool block);

    net::UserCmd& getCurrentUsercmd(void);
    const net::UserCmd& getCurrentUsercmd(void) const;

private:
    void resetCmd(void);
    void mouseMove(void);
    void keyMove(void);
    void setButtonFlags(void);
    void processInput(void);

    // IInputEventListner
    bool OnInputEvent(const input::InputEvent& event) X_FINAL;
    // ~IInputEventListner

    void OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam) X_FINAL;

private:
    static UserButton::Enum getUserButton(input::KeyId::Enum key);

    void setButtonState(input::KeyId::Enum key, bool down);
    int32_t buttonState(UserButton::Enum but) const;

private:
    net::UserCmd cmd_;

    Vec2f mouseDelta_;
    Vec3f viewAngles_;

    input::InputEventBuffer inputEvents_;

    std::array<int32_t, UserButton::ENUM_COUNT> buttonStates_;
    std::array<bool, input::KeyId::ENUM_COUNT> keyState_;
};

X_NAMESPACE_END