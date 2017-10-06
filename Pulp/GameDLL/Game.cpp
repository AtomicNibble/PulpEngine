#include "stdafx.h"
#include "Game.h"

#include <IRender.h>
#include <IConsole.h>
#include <IFrameData.h>

#include <Math\XMatrixAlgo.h>


X_NAMESPACE_BEGIN(game)

namespace
{
	static const Vec3f s_DefaultCamPosition(0, -150, 150);
	static const Vec3f s_DefaultCamAngle(toRadians(-45.f), 0, toRadians(0.f));


}

XGame::XGame(ICore* pCore) :
	arena_(g_gameArena),
	pCore_(pCore),
	pTimer_(nullptr),
	pRender_(nullptr),
	pFovVar_(nullptr),
	world_(arena_),
	localClientId_(entity::INVALID_ID),
	weaponDefs_(arena_)
{
	X_ASSERT_NOT_NULL(pCore);

}

XGame::~XGame()
{

}

void XGame::registerVars(void)
{
	vars_.registerVars();


	// register some vars
//	ADD_CVAR_REF_VEC3("cam_pos", cameraPos_, s_DefaultCamPosition, core::VarFlag::CHEAT,
//		"camera position");
//	ADD_CVAR_REF_VEC3("cam_angle", cameraAngle_, s_DefaultCamAngle, core::VarFlag::CHEAT,
//		"camera angle(radians)");

	pFovVar_ = ADD_CVAR_FLOAT("cam_fov", ::toDegrees(DEFAULT_FOV), 0.01f, ::toDegrees(PIf),
		core::VarFlag::SAVE_IF_CHANGED, "camera fov");

	core::ConsoleVarFunc del;
	del.Bind<XGame, &XGame::OnFovChanged>(this);
	pFovVar_->SetOnChangeCallback(del);


}

void XGame::registerCmds(void)
{
	ADD_COMMAND_MEMBER("map", this, XGame, &XGame::Command_Map, core::VarFlag::SYSTEM, "Loads a map");
//	ADD_COMMAND_MEMBER("devmap", this, XGame, &XGame::Command_DevMap, core::VarFlag::SYSTEM, "Loads a map in developer mode");

}


bool XGame::init(void)
{
	X_LOG0("Game", "init");
	X_ASSERT_NOT_NULL(gEnv->pInput);
	X_ASSERT_NOT_NULL(gEnv->pTimer);
	X_ASSERT_NOT_NULL(gEnv->pRender);

	pTimer_ = gEnv->pTimer;
	pRender_ = gEnv->pRender;


	auto deimension = gEnv->pRender->getDisplayRes();

	X_ASSERT(deimension.x > 0, "height is not valid")(deimension.x);
	X_ASSERT(deimension.y > 0, "height is not valid")(deimension.y);

	cam_.setFrustum(deimension.x, deimension.y, DEFAULT_FOV, 1.f, 2048.f);

	// fiuxed for now, will match network id or something later
	localClientId_ = 0;


	userCmdGen_.init();
	weaponDefs_.init();


	return true;
}

bool XGame::shutDown(void)
{
	X_LOG0("Game", "Shutting Down");

	if (pFovVar_) {
		pFovVar_->Release();
	}

	userCmdGen_.shutdown();
	weaponDefs_.shutDown();
	return true;
}

void XGame::release(void)
{
	X_DELETE(this, g_gameArena);
}

bool XGame::asyncInitFinalize(void)
{
	bool allOk = true;

	allOk &= weaponDefs_.asyncInitFinalize();
	
	return allOk;
}


bool XGame::update(core::FrameData& frame)
{
	X_PROFILE_BEGIN("Update", core::profiler::SubSys::GAME);
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

	Angles<float> goat;

	auto quat = goat.toQuat();
	auto foward = goat.toForward();

	userCmdGen_.buildUserCmd();
	auto& userCmd = userCmdGen_.getCurrentUsercmd();

	// so i want to store the input for what ever player we are but maybe I don't event have a level yet.
	// but we will likley want players before we have a level
	// for lobbies and stuff.
	// i kinda wanna clear all ents when you change level, which is why it's part of the world currently.
	// but makes it annoying to persist shit.
	// what if we just have diffrent registries?
	// one for players lol.
	// or should all this logic only happen if you in a bucket?

	userCmdMan_.addUserCmdForPlayer(localClientId_, userCmd);

	// do we actually have a player?
	// aka is a level loaded shut like that.
	// if not nothing todo.
	if (world_) {
		world_->update(frame, userCmdMan_);
	}
	else
	{
		// orth
		Matrix44f orthoProj;
		MatrixOrthoOffCenterRH(&orthoProj, 0, frame.view.viewport.getWidthf(), frame.view.viewport.getHeightf(), 0, -1e10f, 1e10);

		frame.view.viewMatrixOrtho = Matrix44f::identity();
		frame.view.projMatrixOrtho = orthoProj;
		frame.view.viewProjMatrixOrth = frame.view.viewMatrixOrtho * orthoProj;
	}

//	ProcessInput(frame.timeInfo);

//	cam_.setAngles(cameraAngle_);
//	cam_.setPosition(cameraPos_);

#if 0
	// Pro
	frame.view.cam = cam_;
	frame.view.projMatrix = cam_.getProjectionMatrix();
	frame.view.viewMatrix = cam_.getViewMatrix();
	frame.view.viewProjMatrix = frame.view.viewMatrix * frame.view.projMatrix;
	frame.view.viewProjInvMatrix = frame.view.viewProjMatrix.inverted();
#endif

	// orth
	Matrix44f orthoProj;
	MatrixOrthoOffCenterRH(&orthoProj, 0, frame.view.viewport.getWidthf(), frame.view.viewport.getHeightf(), 0, -1e10f, 1e10);

	frame.view.viewMatrixOrtho = Matrix44f::identity();
	frame.view.projMatrixOrtho = orthoProj;
	frame.view.viewProjMatrixOrth = frame.view.viewMatrixOrtho * orthoProj;


	return true;
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

#if 0
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
#endif

		// I want movement to be relative to the way the camera is facing.
		// so if i'm looking 90 to the right the direction also needs to be rotated.
		Matrix33f angle = Matrix33f::createRotation(cameraAngle_);

		cameraPos_ += (angle * posDelta);
	}

	inputEvents_.clear();
}

void XGame::OnFovChanged(core::ICVar* pVar)
{
	float fovDegress = pVar->GetFloat();
	float fov = ::toRadians(fovDegress);

	cam_.setFov(fov);
}


void XGame::Command_Map(core::IConsoleCmdArgs* Cmd)
{
	if (Cmd->GetArgCount() != 2)
	{
		X_WARNING("Game", "map <mapname>");
		return;
	}

	const char* pMapName = Cmd->GetArg(1);

	world_ = core::makeUnique<World>(arena_, vars_, gEnv->pPhysics, userCmdMan_, weaponDefs_, arena_);

	if (world_) {
		if (!world_->loadMap(pMapName)) {
			X_ERROR("Game", "Failed to load map");
		}
	}
}



X_NAMESPACE_END