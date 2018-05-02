#pragma once

X_NAMESPACE_BEGIN(net)

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

X_DECLARE_ENUM8(Impulse)
(
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
    WEAP_PREV);

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

    Impulse::Enum impulse;
    uint8_t impulseSeq;
};

X_NAMESPACE_END
