#pragma once

#include <Math\XAngles.h>
#include <Time\TimeVal.h>

X_NAMESPACE_DECLARE(core,
    class FixedBitStreamBase)

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

X_DECLARE_FLAGS8(UserCmdFlag)
(
    REPLAY);

typedef Flags8<Button> Buttons;
typedef Flags8<UserCmdFlag> UserCmdFlags;


// this is a set of user commands, it's what we send upto the server.
// so things like movement and fire.
struct UserCmd
{
    void clear() {
        core::zero_this(this);
    }

    void writeToBitStream(core::FixedBitStreamBase& bs) const;
    void fromBitStream(core::FixedBitStreamBase& bs);

    UserCmdFlags flags;
    char _pad[3];

    int32_t clientGameTimeMS;
    int32_t serverGameTimeMS;

    int16_t moveForwrd;
    int16_t moveRight;

    Buttons buttons;

    Anglesf angles;
    Vec2<uint16_t> mouseDelta;

    Impulse::Enum impulse;
    uint8_t impulseSeq;
};

X_NAMESPACE_END
