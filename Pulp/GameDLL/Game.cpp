#include "stdafx.h"
#include "Game.h"

#include <IRender.h>
#include <IConsole.h>
#include <IFrameData.h>

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

bool XGame::init(void)
{
	X_LOG0("Game", "init");
	X_ASSERT_NOT_NULL(gEnv->pInput);
	X_ASSERT_NOT_NULL(gEnv->pTimer);
	X_ASSERT_NOT_NULL(gEnv->pRender);

	gEnv->pInput->AddEventListener(this);
	pTimer_ = gEnv->pTimer;
	pRender_ = gEnv->pRender;


	auto deimension = gEnv->pRender->getDisplayRes();

	X_ASSERT(deimension.x > 0, "height is not valid")(deimension.x);
	X_ASSERT(deimension.y > 0, "height is not valid")(deimension.y);

	cam_.SetFrustum(deimension.x, deimension.y, DEFAULT_FOV, 0.25f, 512.f);

	return true;
}

bool XGame::shutDown(void)
{
	X_LOG0("Game", "Shutting Down");

	pCore_->GetIInput()->RemoveEventListener(this);

	if (pFovVar_) {
		pFovVar_->Release();
	}

	return true;
}

bool XGame::update(core::FrameData& frame)
{
	X_PROFILE_BEGIN("Update", core::ProfileSubSys::GAME);
	X_UNUSED(frame);
	// how todo this camera move shit.
	// when the input frames are been called
	// the frame data has valid times.
	// we just don't have the data in the input callback.
	
	// the real issue is that input callbacks are global events, when this update
	// is a data based call.
	// but i have all the input events in this 
	// but they are no use since i don't know if i'm allowed to use them all.
	// i likethe input sinks tho
	// as things are registerd with priority
	// and each devices gets input events it's allowed to use.
	// the problem is this data not linked to framedata
	// so 
	ProcessInput(frame.timeInfo);

	cam_.setAngles(cameraAngle_);
	cam_.setPosition(cameraPos_);

	frame.view.cam = cam_;
	frame.view.projMatrix = cam_.getProjectionMatrix();
	frame.view.viewMatrix = cam_.getViewMatrix();
	frame.view.viewProjMatrix = frame.view.projMatrix * frame.view.viewMatrix;
	frame.view.viewProjInvMatrix = frame.view.viewProjMatrix.inverted();


//	pRender_->SetCamera(cam_);

	return true;
}


void XGame::registerVars(void)
{

	// register some vars
	ADD_CVAR_REF_VEC3("cam_pos", cameraPos_, s_DefaultCamPosition, core::VarFlag::CHEAT,
		"camera position");
	ADD_CVAR_REF_VEC3("cam_angle", cameraAngle_, s_DefaultCamAngle, core::VarFlag::CHEAT,
		"camera angle(radians)");

	pFovVar_ = ADD_CVAR_FLOAT("cam_fov", ::toDegrees(DEFAULT_FOV), 0.0001f, ::toDegrees(PIf),
		core::VarFlag::SAVE_IF_CHANGED, "camera fov");

	core::ConsoleVarFunc del;
	del.Bind<XGame, &XGame::OnFovChanged>(this);
	pFovVar_->SetOnChangeCallback(del);


}

void XGame::registerCmds(void)
{

}


void XGame::release(void)
{
	X_DELETE(this, g_gameArena);
}


void XGame::ProcessInput(core::FrameTimeData& timeInfo)
{
	X_UNUSED(timeInfo);

	const float speed = 250.f;
	const float timeScale = speed * timeInfo.deltas[core::ITimer::Timer::GAME].GetSeconds();

	for (const auto& e : inputEvents_)
	{
		Vec3f posDelta;

		// rotate.
		switch (e.keyId)
		{
		case input::KeyId::MOUSE_X:
			cameraAngle_.z += -(e.value * 0.002f);
			continue;
		case input::KeyId::MOUSE_Y:
			cameraAngle_.x += -(e.value * 0.002f);
			continue;
		default:
			break;
		}

		float scale = 1.f;
		if (e.modifiers.IsSet(input::InputEvent::ModiferType::LSHIFT)) {
			scale = 2.f;
		}

		scale *= timeScale;

		switch (e.keyId)
		{
			// forwards.
		case input::KeyId::W:
			posDelta.y = scale;
			break;
			// backwards
		case input::KeyId::A:
			posDelta.x = -scale;
			break;

			// Left
		case input::KeyId::S:
			posDelta.y = -scale;
			break;
			// right
		case input::KeyId::D:
			posDelta.x = scale;
			break;

			// up / Down
		case input::KeyId::Q:
			posDelta.z = -scale;
			break;
		case input::KeyId::E:
			posDelta.z = scale;
			break;

		default:
			continue;
		}

		// I want movement to be relative to the way the camera is facing.
		// so if i'm looking 90 to the right the direction also needs to be rotated.
		Matrix33f angle = Matrix33f::createRotation(cameraAngle_);

		cameraPos_ += (angle * posDelta);
	}

	inputEvents_.clear();
}

bool XGame::OnInputEvent(const input::InputEvent& event)
{
	// theses event have pointers to symbols that will change in next input poll
	// but we don't use any of the data from symbol.
	inputEvents_.emplace_back(event);
	return false;
}

bool XGame::OnInputEventChar(const input::InputEvent& event)
{
	X_UNUSED(event);

	return false;
}

void XGame::OnFovChanged(core::ICVar* pVar)
{
	float fovDegress = pVar->GetFloat();
	float fov = ::toRadians(fovDegress);

	cam_.setFov(fov);
}




X_NAMESPACE_END