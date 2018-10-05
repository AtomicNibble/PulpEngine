#include "stdafx.h"
#include "PlayerVars.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(game)

PlayerVars::PlayerVars()
{
    gunOffset_ = Vec3f(3.f, 0.f, 0.f);

    drawPosInfo_ = 0;

    thirdPerson_ = 0;
    thirdPersonRange_ = 20;
    thirdPersonAngle_ = 0;

    jumpHeight_ = 50.f;
    crouchHeight_ = 30.f;
    crouchViewHeight_ = 40.f;
    normalHeight_ = 50.f;
    normalViewHeight_ = 70.f;
    crouchRate_ = 0.8f;

    maxViewPitch_ = 89;
    minViewPitch_ = -89;

    walkBob_ = 0.4f;
    runBob_ = 0.5f;
    crouchBob_ = 0.6f;

    bobUp_ = 0.005f;
    bobPitch_ = 0.002f;
    bobRoll_ = 0.002f;
    bobRunPitch_ = 0.002f;
    bobRunRoll_ = 0.005f;

    walkSpeed_ = 150;
    runSpeed_ = 220;
    crouchSpeed_ = 80;
}

void PlayerVars::registerVars(void)
{
    ADD_CVAR_REF_VEC3("ply_gun_offset", gunOffset_, gunOffset_, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED, "");

    ADD_CVAR_REF("ply_draw_pos", drawPosInfo_, drawPosInfo_, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
        "Draw current position & angle");

    ADD_CVAR_REF("ply_thirdperson", thirdPerson_, thirdPerson_, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::CHEAT,
        "One dimension to rule them all");

    ADD_CVAR_REF("ply_thirdperson_range", thirdPersonRange_, thirdPersonRange_, 0, 256, core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
        "The distance of the 3rd person camera from player");
    ADD_CVAR_REF("ply_thirdperson_angle", thirdPersonAngle_, thirdPersonAngle_, 0, 360, core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
        "Direction of 3rd person camera in degrees. (0: behind player)");

    ADD_CVAR_REF("ply_jump_height", jumpHeight_, jumpHeight_, 0, 256, core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
        "MAx height player can jump");
    ADD_CVAR_REF("ply_crouch_view_height", crouchViewHeight_, crouchViewHeight_, 0, 256, core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
        "Height of player when crouched");
    ADD_CVAR_REF("ply_crouch_height", crouchHeight_, crouchHeight_, 0, 256, core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
        "Height of player collsion shape when crouched");
    ADD_CVAR_REF("ply_view_height", normalViewHeight_, normalViewHeight_, 0, 256, core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
        "Height of player view");
    ADD_CVAR_REF("ply_height", normalHeight_, normalHeight_, 0, 256, core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
        "Height of player's collsion shape (Must crouch to apply)");
    ADD_CVAR_REF("ply_crouch_rate", crouchRate_, crouchRate_, 0, 1.f, core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
        "Time it takes to move in and out of crouch");

    ADD_CVAR_REF("ply_max_view_pitch", maxViewPitch_, maxViewPitch_, -360, 360, core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
        "Max view pitch angle");
    ADD_CVAR_REF("ply_min_view_pitch", minViewPitch_, minViewPitch_, -360, 360, core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
        "Min view pitch angle");

    ADD_CVAR_REF("ply_walk_bob", walkBob_, walkBob_, 0, 512, core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
        "Walk camera bob");
    ADD_CVAR_REF("ply_run_bob", runBob_, runBob_, 0, 512, core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
        "Run camera bob");
    ADD_CVAR_REF("ply_crouch_bob", crouchBob_, crouchBob_, 0, 512, core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
        "Crouch camera bob");

    ADD_CVAR_REF("ply_bob_up", bobUp_, bobUp_, 0, 1.f, core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
        "");
    ADD_CVAR_REF("ply_bob_pitch", bobPitch_, bobPitch_, 0, 512, core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
        "");
    ADD_CVAR_REF("ply_bob_roll", bobRoll_, bobRoll_, 0, 512, core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
        "");
    ADD_CVAR_REF("ply_bob_run_pitch", bobRunPitch_, bobRunPitch_, 0, 512, core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
        "");
    ADD_CVAR_REF("ply_bob_run_roll", bobRunRoll_, bobRunRoll_, 0, 512, core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
        "");

    ADD_CVAR_REF("ply_walk_speed", walkSpeed_, walkSpeed_, 0, 512, core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
        "Move speed when walking");
    ADD_CVAR_REF("ply_run_speed", runSpeed_, runSpeed_, 0, 512, core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
        "Move speed when running");
    ADD_CVAR_REF("ply_crouch_speed", crouchSpeed_, crouchSpeed_, 0, 512, core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
        "Move speed when crouched");
}

X_NAMESPACE_END