#include "stdafx.h"
#include "Game.h"


#include <IRender.h>

X_NAMESPACE_BEGIN(game)

XGame::XGame(ICore* pCore) :
pCore_(pCore)
{
	X_ASSERT_NOT_NULL(pCore);

}

XGame::~XGame()
{

}

bool XGame::Init(void)
{
	X_LOG0("Game", "init");
	pCore_->GetIInput()->AddEventListener(this);


	// static const Ang3	s_angDefaultCam(DEG2RAD( -10 ), 0, DEG2RAD( 17 ));

	cameraAngle_ = Vec3f(0,0,0);

	return true;
}

bool XGame::ShutDown(void)
{
	X_LOG0("Game", "Shutting down");

	pCore_->GetIInput()->RemoveEventListener(this);


	return true;
}

bool XGame::Update(void)
{
	X_PROFILE_BEGIN("Update", core::ProfileSubSys::GAME);

	core::StackString<256> txt;
	render::XDrawTextInfo ti;
	ti.col = Col_Whitesmoke;
	//	ti.flags;

	txt.appendFmt("Pos: (%f, %f, %f)\n", cameraPos_.x, cameraPos_.y, cameraPos_.z);
	txt.appendFmt("Angle (%f,%f,%f)", cameraAngle_.x, cameraAngle_.y, cameraAngle_.z);

	gEnv->pRender->DrawTextQueued(Vec3f(10, 40, 0), ti, txt.c_str());

	XCamera cam;
	cam.SetFrustum(800,600);
	cam.SetAngles(cameraAngle_);
	cam.SetPosition(cameraPos_);

	gEnv->pRender->SetCamera(cam);

	return true;
}


bool XGame::OnInputEvent(const input::InputEvent& event)
{
	X_UNUSED(event);
	using namespace input;

	Vec3f posDelta;

	// rotate.
	switch (event.keyId)
	{
		case KeyId::MOUSE_X:
			cameraAngle_.z += -(event.value * 0.005f);
			return false;
		case KeyId::MOUSE_Y:
			cameraAngle_.x += -(event.value * 0.005f);
			return false;
		default:
		break;
	}

	const float moveDelta = 40.0f;
	switch (event.keyId)		
	{
		// forwards.
		case KeyId::W:
		posDelta.y = moveDelta;
		break;
		// backwards
		case KeyId::A:
		posDelta.x = -moveDelta;
		break;

		// Left
		case KeyId::S:
		posDelta.y = -moveDelta;
		break;
		// right
		case KeyId::D:
		posDelta.x = moveDelta;
		break;
	
		// up / Down
		case KeyId::Q:
		posDelta.z = -moveDelta;
		break;
		case KeyId::E:
		posDelta.z = moveDelta;
		break;

		default:
			break;
	}

	// I want movement to be relative to the way the camera is facing.
	// so if i'm looking 90 to the right the direction also needs to be rotated.
	Matrix33f angle = Matrix33f::createRotation(cameraAngle_);

	cameraPos_ += (angle * posDelta);
	return false;
}

bool XGame::OnInputEventChar(const input::InputEvent& event)
{
	X_UNUSED(event);

	return false;
}


X_NAMESPACE_END