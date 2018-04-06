#include "stdafx.h"
#include "UserCmds.h"

X_NAMESPACE_BEGIN(game)

UserCmdGen::UserCmdGen()
{
    clear();
}

bool UserCmdGen::init(void)
{
    gEnv->pInput->AddEventListener(this);

    return true;
}

void UserCmdGen::shutdown(void)
{
    gEnv->pInput->RemoveEventListener(this);
}

void UserCmdGen::clear(void)
{
    viewAngles_ = Vec3f::zero();
    mouseDelta_ = Vec2f::zero();

    buttonStates_.fill(0);
    keyState_.fill(false);
}

void UserCmdGen::buildUserCmd(void)
{
    resetCmd();

    processInput();

    setButtonFlags();

    mouseMove();
    keyMove();

    cmd_.angles = Anglesf(viewAngles_);
}

UserCmd& UserCmdGen::getCurrentUsercmd(void)
{
    return cmd_;
}

const UserCmd& UserCmdGen::getCurrentUsercmd(void) const
{
    return cmd_;
}

void UserCmdGen::resetCmd(void)
{
    core::zero_object(cmd_);
}

void UserCmdGen::mouseMove(void)
{
    // do things like scaling or inverting pitch etc..
    // ..

    viewAngles_[Rotation::YAW] -= mouseDelta_.x;
    viewAngles_[Rotation::PITCH] -= mouseDelta_.y;

    mouseDelta_ = Vec2f::zero();

    Vec3f oldAngles = viewAngles_;

    // check to make sure the angles haven't wrapped
    if (viewAngles_[Rotation::PITCH] - oldAngles[Rotation::PITCH] > 90) {
        viewAngles_[Rotation::PITCH] = oldAngles[Rotation::PITCH] + 90;
    }
    else if (oldAngles[Rotation::PITCH] - viewAngles_[Rotation::PITCH] > 90) {
        viewAngles_[Rotation::PITCH] = oldAngles[Rotation::PITCH] - 90;
    }
}

void UserCmdGen::keyMove(void)
{
    int32_t forward = 0;
    int32_t side = 0;

    side += buttonState(UserButton::MOVE_RIGHT);
    side -= buttonState(UserButton::MOVE_LEFT);

    forward += buttonState(UserButton::MOVE_FORWARD);
    forward -= buttonState(UserButton::MOVE_BACK);

    cmd_.moveForwrd += safe_static_cast<int16_t>(forward);
    cmd_.moveRight += safe_static_cast<int16_t>(side);
}

void UserCmdGen::setButtonFlags(void)
{
    cmd_.buttons.Clear();

    if (buttonState(UserButton::ATTACK)) {
        cmd_.buttons.Set(Button::ATTACK);
    }
    if (buttonState(UserButton::USE)) {
        cmd_.buttons.Set(Button::USE);
    }

    if (buttonState(UserButton::MOVE_UP)) {
        cmd_.buttons.Set(Button::JUMP);
    }
    if (buttonState(UserButton::MOVE_DOWN)) {
        cmd_.buttons.Set(Button::CROUCH);
    }

    if (buttonState(UserButton::SPEED)) {
        cmd_.buttons.Set(Button::RUN);
    }

    if (buttonState(UserButton::RELOAD)) {
        cmd_.buttons.Set(Button::RELOAD);
    }
}

void UserCmdGen::processInput(void)
{
    for (const auto& e : inputEvents_) {
        if (e.deviceType == input::InputDeviceType::KEYBOARD) {
            // want to map user keys to defined commands.
            // for now just hard code.
            setButtonState(e.keyId, (e.action == input::InputState::DOWN));
        }
        else if (e.deviceType == input::InputDeviceType::MOUSE) {
            switch (e.keyId) {
                case input::KeyId::MOUSE_X:
                    mouseDelta_.x += (e.value * 0.02f);
                    break;
                case input::KeyId::MOUSE_Y:
                    mouseDelta_.y += (e.value * 0.02f);
                    break;

                default:
                    setButtonState(e.keyId, (e.action == input::InputState::PRESSED));
                    break;
            }
        }
    }

    inputEvents_.clear();
}

UserButton::Enum getUserButton(input::KeyId::Enum key)
{
    // TODO: map these to key bindings.
    switch (key) {
        case input::KeyId::F:
            return UserButton::USE;
        case input::KeyId::MOUSE_LEFT:
            return UserButton::ATTACK;
        case input::KeyId::MOUSE_RIGHT:
            return UserButton::ZOOM;
        case input::KeyId::R:
            return UserButton::RELOAD;

        case input::KeyId::W:
            return UserButton::MOVE_FORWARD;
        case input::KeyId::S:
            return UserButton::MOVE_BACK;
        case input::KeyId::D:
            return UserButton::MOVE_RIGHT;
        case input::KeyId::A:
            return UserButton::MOVE_LEFT;
        case input::KeyId::LEFT_CONTROL:
            return UserButton::MOVE_DOWN;
        case input::KeyId::SPACEBAR:
            return UserButton::MOVE_UP;

        case input::KeyId::LEFT_SHIFT:
            return UserButton::SPEED;

        default:
            return UserButton::NONE;
    }
}

void UserCmdGen::setButtonState(input::KeyId::Enum key, bool down)
{
    if (keyState_[key] == down) {
        return;
    }
    keyState_[key] = down;

    auto ub = getUserButton(key);
    if (down) {
        ++buttonStates_[ub];
    }
    else {
        --buttonStates_[ub];

        if (buttonStates_[ub] < 0) {
            buttonStates_[ub] = 0;
        }
    }
}

int32_t UserCmdGen::buttonState(UserButton::Enum but) const
{
    return buttonStates_[but];
}

bool UserCmdGen::OnInputEvent(const input::InputEvent& event)
{
    inputEvents_.emplace_back(event);
    return false;
}

bool UserCmdGen::OnInputEventChar(const input::InputEvent& event)
{
    X_UNUSED(event);
    return false;
}

X_NAMESPACE_END