#pragma once

X_NAMESPACE_BEGIN(game)

X_DECLARE_ENUM(Rotation)
(
    PITCH,
    YAW,
    ROLL);

X_DECLARE_FLAGS8(Button)
(
    JUMP,
    CROUCH,
    ATTACK,
    RELOAD,
    ZOOM,
    RUN,
    USE);

typedef Flags8<Button> Buttons;

// this is a set of user commands, it's what we send upto the server.
// so things like movement and fire.
struct UserCmd
{
    int16_t moveForwrd;
    int16_t moveRight;

    Buttons buttons;

    Anglesf angles;
    Vec2<uint16_t> mouseDelta;
};

X_NAMESPACE_END