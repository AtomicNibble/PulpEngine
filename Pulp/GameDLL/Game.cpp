#include "stdafx.h"
#include "Game.h"

#include <IRender.h>
#include <IConsole.h>

X_NAMESPACE_BEGIN(game)

namespace
{
	static const Vec3f s_DefaultCamPosition(0, -150, 150);
	static const Vec3f s_DefaultCamAngle(toRadians(-45.f), 0, toRadians(0.f));


}

XGame::XGame(ICore* pCore) :
pCore_(pCore),
pTimer_(nullptr),
pRender_(nullptr),
pFovVar_(nullptr)
{
	X_ASSERT_NOT_NULL(pCore);

}

XGame::~XGame()
{

}

bool XGame::Init(void)
{
	X_LOG0("Game", "init");
	X_ASSERT_NOT_NULL(gEnv->pInput);
	X_ASSERT_NOT_NULL(gEnv->pTimer);
	X_ASSERT_NOT_NULL(gEnv->pRender);

	gEnv->pInput->AddEventListener(this);
	pTimer_ = gEnv->pTimer;
	pRender_ = gEnv->pRender;

	timeLast_ = pTimer_->GetAsyncTime();

	// register some vars
	ADD_CVAR_REF_VEC3("cam_pos", cameraPos_, s_DefaultCamPosition, core::VarFlag::STATIC, 
		"camera position");
	ADD_CVAR_REF_VEC3("cam_angle", cameraAngle_, s_DefaultCamAngle, core::VarFlag::STATIC, 
		"camera angle(radians)");
	pFovVar_ = ADD_CVAR_FLOAT("cam_fov", ::toDegrees(DEFAULT_FOV), 0.0001f, ::toDegrees(PIf), 
		core::VarFlag::SAVE_IF_CHANGED, "camera fov");

	pFovVar_->SetOnChangeCallback(s_OnFovChanged);

	uint32_t width, height;

	height = static_cast<uint32_t>(gEnv->pRender->getHeight());
	width = static_cast<uint32_t>(gEnv->pRender->getWidth());
	
	X_ASSERT(height > 0, "height is not valid")(height);
	X_ASSERT(width > 0, "height is not valid")(width);

	cam_.SetFrustum(width, height, DEFAULT_FOV, 0.25f, 512.f);

	return true;
}

bool XGame::ShutDown(void)
{
	X_LOG0("Game", "Shutting Down");

	pCore_->GetIInput()->RemoveEventListener(this);

	if (pFovVar_) {
		pFovVar_->Release();
	}

	return true;
}

bool XGame::Update(void)
{
	X_PROFILE_BEGIN("Update", core::ProfileSubSys::GAME);



	cam_.setAngles(cameraAngle_);
	cam_.setPosition(cameraPos_);
//	cam_.setFov(cameraFov_);

	pRender_->SetCamera(cam_);

	return true;
}

void XGame::release(void)
{
	X_DELETE(this, g_gameArena);
}

bool XGame::OnInputEvent(const input::InputEvent& event)
{
	X_UNUSED(event);
	using namespace input;

	Vec3f posDelta;
	bool dontUpdate = false;

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

	// we want to move a fixed amount of time.
	// we track how much time has passed since that last update.
	// and use that for delta.

	// timeLast_ = pTimer_->GetAsyncTime() - timeLast_;

	float speed = 250.f;
	if (event.modifiers.IsSet(InputEvent::ModiferType::LSHIFT)) {
		speed *= 2.f;
	}

	float moveDelta = speed * pTimer_->GetFrameTime();
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
		dontUpdate = true;
			break;
	}

	if (dontUpdate)
		return false;

	// I want movement to be relative to the way the camera is facing.
	// so if i'm looking 90 to the right the direction also needs to be rotated.
	Matrix33f angle = Matrix33f::createRotation(cameraAngle_);

	cameraPos_ +=  (angle * posDelta);
	return false;
}

bool XGame::OnInputEventChar(const input::InputEvent& event)
{
	X_UNUSED(event);

	return false;
}

void XGame::s_OnFovChanged(core::ICVar* pVar)
{
	float fovDegress = pVar->GetFloat();
	float fov = ::toRadians(fovDegress);

	XGame* pGame = static_cast<XGame*>(gEnv->pGame);
	pGame->OnFovChanged(fov);
}

void XGame::OnFovChanged(float fov)
{
	cam_.setFov(fov);
}




X_NAMESPACE_END