#include "stdafx.h"
#include "UserCmds.h"

X_NAMESPACE_BEGIN(game)

UserCmdGen::UserCmdGen()
{
	moveForward_ = 0;
	moveRight_ = 0;

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
}

void UserCmdGen::buildUserCmd(void)
{
	resetCmd();

	processInput();

	mouseMove();


	cmd_.angles = viewAngles_;
	cmd_.moveForwrd = moveForward_;
	cmd_.moveRight = moveRight_;

}

UserCmd& UserCmdGen::getCurrentUsercmd(void)
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
}



void UserCmdGen::processInput(void)
{

	for (const auto& e : inputEvents_)
	{
		switch (e.keyId)
		{
			case input::KeyId::MOUSE_X:
				mouseDelta_.x += e.value;
				break;
			case input::KeyId::MOUSE_Y:
				mouseDelta_.y += e.value;
				break;


			case input::KeyId::W:
				moveForward_ += 1_i16;
				break;
			case input::KeyId::S:
				moveForward_ += -1_i16;
				break;
			case input::KeyId::D:
				moveRight_ += 1_i16;
				break;
			case input::KeyId::A:
				moveRight_ += -1_i16;
				break;


			default:
				break;
		}



	}

	inputEvents_.clear();
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