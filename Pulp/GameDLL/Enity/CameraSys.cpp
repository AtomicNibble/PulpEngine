#include "stdafx.h"
#include "CameraSys.h"
#include <IRender.h> // temp

#include <IFrameData.h>
#include <IConsole.h>

X_NAMESPACE_BEGIN(game)

namespace entity
{


	CameraSystem::CameraSystem() :
		activeEnt_(EnitiyRegister::INVALID_ID)
	{

	}

	CameraSystem::~CameraSystem()
	{

	}

	bool CameraSystem::init(void)
	{
		auto deimension = gEnv->pRender->getDisplayRes();

		cam_.SetFrustum(deimension.x, deimension.y, DEFAULT_FOV, 1.f, 2048.f);


		ADD_CVAR_REF_VEC3("cam_pos", cameraPos_, cameraPos_, core::VarFlag::CHEAT | core::VarFlag::READONLY,
			"camera position");
		ADD_CVAR_REF_VEC3("cam_angle", cameraAngleDeg_, cameraAngleDeg_, core::VarFlag::CHEAT | core::VarFlag::READONLY,
			"camera angle(radians)");

		return true;
	}

	void CameraSystem::update(core::FrameData& frame, EnitiyRegister& reg, physics::IScene* pPhysScene)
	{
		X_UNUSED(pPhysScene);

		if (reg.isValid(activeEnt_))
		{
			if (reg.has<TransForm>(activeEnt_))
			{
				auto& trans = reg.get<TransForm>(activeEnt_);

				cameraPos_ = trans.trans.pos;
				cameraPos_ += Vec3f(0, 0, 20.f);
				cameraAngle_ = trans.trans.quat.getEuler();
				
		//		cameraAngle_.y = 0.f;

				cameraAngleDeg_.x = ::toDegrees(cameraAngle_.x);
				cameraAngleDeg_.y = ::toDegrees(cameraAngle_.y);
				cameraAngleDeg_.z = ::toDegrees(cameraAngle_.z);
			}
			else if (reg.has<Position>(activeEnt_))
			{
				auto& pos = reg.get<Position>(activeEnt_);

				cameraPos_ = pos.pos;
			}
			else
			{
				X_ASSERT_UNREACHABLE();
			}
		}

		cam_.setAngles(cameraAngle_);
		cam_.setPosition(cameraPos_);

		// Pro
		frame.view.cam = cam_;
		frame.view.projMatrix = cam_.getProjectionMatrix();
		frame.view.viewMatrix = cam_.getViewMatrix();
		frame.view.viewProjMatrix = frame.view.viewMatrix * frame.view.projMatrix;
		frame.view.viewProjInvMatrix = frame.view.viewProjMatrix.inverted();
	}

	void CameraSystem::setActiveEnt(EntityId entId)
	{
		activeEnt_ = entId;
	}


} // namespace entity

X_NAMESPACE_END