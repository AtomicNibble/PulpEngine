#include "stdafx.h"
#include "PlayerVars.h"

#include <IConsole.h>


X_NAMESPACE_BEGIN(game)


PlayerVars::PlayerVars()
{
	thirdPerson_ = 0;
	thirdPersonRange_ = 20;
	thirdPersonAngle_ = 0;

	jumpHeight_ = 50.f;
	crouchHeight_ = 30.f;
	crouchViewHeight_ = 20.f;
	normalHeight_ = 50.f;
	normalViewHeight_ = 40.f;
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
	ADD_CVAR_REF("p_thirdperson", thirdPerson_, thirdPerson_, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::CHEAT, 
		"One dimension to rule them all");

	ADD_CVAR_REF("p_thirdperson_range", thirdPersonRange_, thirdPersonRange_, 0, 256, core::VarFlag::SYSTEM | core::VarFlag::CHEAT,
		"The distance of the 3rd person camera from player");
	ADD_CVAR_REF("p_thirdperson_angle", thirdPersonAngle_, thirdPersonAngle_, 0, 360, core::VarFlag::SYSTEM | core::VarFlag::CHEAT,
		"Direction of 3rd person camera in degrees. (0: behind player)");

	ADD_CVAR_REF("p_jump_height", jumpHeight_, jumpHeight_, 0, 256, core::VarFlag::SYSTEM | core::VarFlag::CHEAT,
		"MAx height player can jump");
	ADD_CVAR_REF("p_crouch_view_height", crouchViewHeight_, crouchViewHeight_, 0, 256, core::VarFlag::SYSTEM | core::VarFlag::CHEAT,
		"Height of player when crouched");
	ADD_CVAR_REF("p_crouch_height", crouchHeight_, crouchHeight_, 0, 256, core::VarFlag::SYSTEM | core::VarFlag::CHEAT,
		"Height of player collsion shape when crouched");
	ADD_CVAR_REF("p_normal_view_height", normalViewHeight_, normalViewHeight_, 0, 256, core::VarFlag::SYSTEM | core::VarFlag::CHEAT,
		"Height of player view");
	ADD_CVAR_REF("p_normal_height", normalHeight_, normalHeight_, 0, 256, core::VarFlag::SYSTEM | core::VarFlag::CHEAT,
		"Height of player's collsion shape");
	ADD_CVAR_REF("p_crouch_rate", crouchRate_, crouchRate_, 0, 1.f, core::VarFlag::SYSTEM | core::VarFlag::CHEAT,
		"Time it takes to move in and out of crouch");

	ADD_CVAR_REF("p_max_view_pitch", maxViewPitch_, maxViewPitch_, -360, 360, core::VarFlag::SYSTEM | core::VarFlag::CHEAT,
		"");
	ADD_CVAR_REF("p_min_view_pitch", minViewPitch_, minViewPitch_, -360, 360, core::VarFlag::SYSTEM | core::VarFlag::CHEAT,
		"");

	ADD_CVAR_REF("p_walk_bob", walkBob_, walkBob_, 0, 512, core::VarFlag::SYSTEM | core::VarFlag::CHEAT,
		"");
	ADD_CVAR_REF("p_run_bob", runBob_, runBob_, 0, 512, core::VarFlag::SYSTEM | core::VarFlag::CHEAT,
		"");
	ADD_CVAR_REF("p_crouch_bob", crouchBob_, crouchBob_, 0, 512, core::VarFlag::SYSTEM | core::VarFlag::CHEAT,
		"");

	ADD_CVAR_REF("p_bob_up", bobUp_, bobUp_, 0, 1.f, core::VarFlag::SYSTEM | core::VarFlag::CHEAT,
		"");
	ADD_CVAR_REF("p_bob_pitch", bobPitch_, bobPitch_, 0, 512, core::VarFlag::SYSTEM | core::VarFlag::CHEAT,
		"");
	ADD_CVAR_REF("p_bob_roll", bobRoll_, bobRoll_, 0, 512, core::VarFlag::SYSTEM | core::VarFlag::CHEAT,
		"");
	ADD_CVAR_REF("p_bob_run_pitch", bobRunPitch_, bobRunPitch_, 0, 512, core::VarFlag::SYSTEM | core::VarFlag::CHEAT,
		"");
	ADD_CVAR_REF("p_bob_run_roll", bobRunRoll_, bobRunRoll_, 0, 512, core::VarFlag::SYSTEM | core::VarFlag::CHEAT,
		"");


	ADD_CVAR_REF("p_walk_speed", walkSpeed_, walkSpeed_, 0, 512, core::VarFlag::SYSTEM | core::VarFlag::CHEAT,
		"Move speed when walking");
	ADD_CVAR_REF("p_run_speed", runSpeed_, runSpeed_, 0, 512, core::VarFlag::SYSTEM | core::VarFlag::CHEAT,
		"Move speed when running");
	ADD_CVAR_REF("p_crouch_speed", crouchSpeed_, crouchSpeed_, 0, 512, core::VarFlag::SYSTEM | core::VarFlag::CHEAT,
		"Move speed when crouched");

}



X_NAMESPACE_END