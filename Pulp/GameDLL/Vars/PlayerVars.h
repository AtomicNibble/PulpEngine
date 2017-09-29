#pragma once

X_NAMESPACE_DECLARE(core,
	struct ICVar;
)

X_NAMESPACE_BEGIN(game)

class PlayerVars
{
public:
	PlayerVars();
	~PlayerVars() = default;

	void registerVars(void);


public:
	Vec3f gunOffset_;

	int32_t thirdPerson_; // enabled 3rd person.
	int32_t thirdPersonRange_;
	int32_t thirdPersonAngle_;

	float jumpHeight_;
	float crouchViewHeight_;
	float crouchHeight_;
	float normalHeight_;
	float normalViewHeight_;
	float crouchRate_;

	float maxViewPitch_;
	float minViewPitch_;

	float walkBob_;
	float runBob_;
	float crouchBob_;

	float bobUp_;
	float bobPitch_;
	float bobRoll_;
	float bobRunPitch_;
	float bobRunRoll_;

	// move speeds
	float walkSpeed_;
	float runSpeed_;
	float crouchSpeed_;
};

X_NAMESPACE_END

#include "PlayerVars.inl"
