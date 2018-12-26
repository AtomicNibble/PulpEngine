#include "stdafx.h"
#include "UserCmdGen.h"

#include "Vars\InputVars.h"

X_NAMESPACE_BEGIN(game)

ButtonState::ButtonState()
{
    clear();
}

void ButtonState::clear(void)
{
    active_ = false;
    held_ = false;
}

void ButtonState::setKeyState(int32_t keystate, bool toggle)
{
    if (!toggle) {
        active_ = keystate;
        held_ = false;
    }
    else if (!keystate) {
        held_ = false;
    }
    else if (!held_) {
        active_ ^= 1;
        held_ = true;
    }
}

bool ButtonState::isActive(void) const
{
    return active_ > 0;
}

// ---------------------

UserCmdGen::UserCmdGen(const InputVars& vars) :
    vars_(vars)
{
    clear();
}

bool UserCmdGen::init(void)
{
    gEnv->pCore->GetCoreEventDispatcher()->RegisterListener(this);

    return true;
}

void UserCmdGen::shutdown(void)
{
    gEnv->pCore->GetCoreEventDispatcher()->RemoveListener(this);
}

void UserCmdGen::clearForNewLevel(void)
{
    toggledCrouch_.clear();
    toggledRun_.clear();
    toggledZoom_.clear();

    clear();
    clearAngles();
}

void UserCmdGen::clear(void)
{
    mouseDelta_ = Vec2f::zero();

    buttonStates_.fill(0);
    keyState_.fill(false);
}

void UserCmdGen::clearAngles(void)
{
    viewAngles_ = Vec3f::zero();
}

bool UserCmdGen::onInputEvent(const input::InputEvent& event)
{
    if (event.action == input::InputState::CHAR) {
        return false;
    }

    if (inputEvents_.size() == inputEvents_.capacity()) {
        X_WARNING("UserCmd", "Input event overflow");
        return false;
    }

    inputEvents_.emplace_back(event);
    return false;
}


void UserCmdGen::buildUserCmd(bool blockInput)
{
    resetCmd();
    processInput();

    if (blockInput)
    {
        mouseDelta_ = Vec2f::zero();
    }
    else
    {
        toggledCrouch_.setKeyState(buttonState(UserButton::MOVE_DOWN), vars_.toggleCrouch());
        toggledRun_.setKeyState(buttonState(UserButton::RUN), vars_.toggleRun());
        toggledZoom_.setKeyState(buttonState(UserButton::ZOOM), vars_.toggleZoom());

        setButtonFlags();

        mouseMove();
        keyMove();
    }

    cmd_.angles = Anglesf(viewAngles_);
}

net::UserCmd& UserCmdGen::getCurrentUserCmd(void)
{
    return cmd_;
}

const net::UserCmd& UserCmdGen::getCurrentUserCmd(void) const
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

    viewAngles_[net::Rotation::YAW] -= mouseDelta_.x;
    viewAngles_[net::Rotation::PITCH] -= mouseDelta_.y;

    mouseDelta_ = Vec2f::zero();

    Vec3f oldAngles = viewAngles_;

    // check to make sure the angles haven't wrapped
    if (viewAngles_[net::Rotation::PITCH] - oldAngles[net::Rotation::PITCH] > 90) {
        viewAngles_[net::Rotation::PITCH] = oldAngles[net::Rotation::PITCH] + 90;
    }
    else if (oldAngles[net::Rotation::PITCH] - viewAngles_[net::Rotation::PITCH] > 90) {
        viewAngles_[net::Rotation::PITCH] = oldAngles[net::Rotation::PITCH] - 90;
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
        cmd_.buttons.Set(net::Button::ATTACK);
    }
    if (buttonState(UserButton::USE)) {
        cmd_.buttons.Set(net::Button::USE);
    }
    if (buttonState(UserButton::MOVE_UP)) {
        cmd_.buttons.Set(net::Button::JUMP);
    }
    if (buttonState(UserButton::RELOAD)) {
        cmd_.buttons.Set(net::Button::RELOAD);
    }
    if (buttonState(UserButton::SHOW_SCORES)) {
        cmd_.buttons.Set(net::Button::SHOW_SCORES);
    }

    // some stuff that can be either toggle or hold.
    if (toggledCrouch_.isActive()) {
        cmd_.buttons.Set(net::Button::CROUCH);
    }
    if (toggledRun_.isActive()) {
        cmd_.buttons.Set(net::Button::RUN);
    }
    if (toggledZoom_.isActive()) {
        cmd_.buttons.Set(net::Button::ZOOM);
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

                case input::KeyId::MOUSE_Z:
                {
                    auto key = e.value < 0.f ? input::KeyId::MOUSE_WHEELDOWN : input::KeyId::MOUSE_WHEELUP;
                    setButtonState(key, true);
                    setButtonState(key, false);
                    break;
                }
                default:
                    setButtonState(e.keyId, (e.action == input::InputState::PRESSED));
                    break;
            }
        }
    }

    inputEvents_.clear();
}

UserButton::Enum UserCmdGen::getUserButton(input::KeyId::Enum key)
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
            return UserButton::RUN;

        case input::KeyId::DIGIT_0:
            return UserButton::WEAP0;
        case input::KeyId::DIGIT_1:
            return UserButton::WEAP1;
        case input::KeyId::DIGIT_2:
            return UserButton::WEAP2;
        case input::KeyId::DIGIT_3:
            return UserButton::WEAP3;
        case input::KeyId::DIGIT_4:
            return UserButton::WEAP4;
        case input::KeyId::DIGIT_5:
            return UserButton::WEAP5;
        case input::KeyId::DIGIT_6:
            return UserButton::WEAP6;
        case input::KeyId::DIGIT_7:
            return UserButton::WEAP7;
        case input::KeyId::DIGIT_8:
            return UserButton::WEAP8;
        case input::KeyId::DIGIT_9:
            return UserButton::WEAP9;

        case input::KeyId::Q:
        case input::KeyId::MOUSE_WHEELDOWN:
            return UserButton::WEAP_NEXT;
        case input::KeyId::E:
        case input::KeyId::MOUSE_WHEELUP:
            return UserButton::WEAP_PREV;

        case input::KeyId::TAB:
            return UserButton::SHOW_SCORES;


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

        // so want to seperate out impulses.
        if (ub >= UserButton::WEAP0) {
            int32_t val = static_cast<int32_t>(ub - UserButton::WEAP0);
            X_ASSERT(val >= 0 && val < net::Impulse::ENUM_COUNT, "Out of range")(val);

            cmd_.impulse = static_cast<net::Impulse::Enum>(val);
            cmd_.impulseSeq++;
        }
    }
    else {
        --buttonStates_[ub];

        if (buttonStates_[ub] < 0) {
            X_ASSERT_UNREACHABLE();
            buttonStates_[ub] = 0;
        }
    }
}

int32_t UserCmdGen::buttonState(UserButton::Enum but) const
{
    return buttonStates_[but];
}

void UserCmdGen::OnCoreEvent(const CoreEventData& ed)
{
    if (ed.event == CoreEvent::CHANGE_FOCUS)
    {
        clear();
    }
}

X_NAMESPACE_END