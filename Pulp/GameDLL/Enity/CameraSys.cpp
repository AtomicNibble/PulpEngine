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

		cam_.setFrustum(deimension.x, deimension.y, DEFAULT_FOV, 1.f, 2048.f);


		ADD_CVAR_REF_VEC3("cam_pos", cameraPos_, cameraPos_, core::VarFlag::CHEAT | core::VarFlag::READONLY,
			"camera position");
		ADD_CVAR_REF_VEC3("cam_angle", cameraAngleDeg_, cameraAngleDeg_, core::VarFlag::CHEAT | core::VarFlag::READONLY,
			"camera angle(radians)");

		ADD_CVAR_REF_VEC3("cam_angle_rad", cameraAngle_, cameraAngle_, core::VarFlag::CHEAT | core::VarFlag::READONLY,
			"camera angle(radians)");

		return true;
	}

	void CameraSystem::update(core::FrameData& frame, EnitiyRegister& reg, physics::IScene* pPhysScene)
	{
		X_UNUSED(pPhysScene);

		Matrix33f mat33;

		if (reg.isValid(activeEnt_))
		{
			if (reg.has<Player>(activeEnt_))
			{
				auto& player = reg.get<Player>(activeEnt_);

				cameraPos_ = player.firstPersonViewOrigin;
				mat33 = player.firstPersonViewAxis;
			}
			else if (reg.has<TransForm>(activeEnt_))
			{
				auto& trans = reg.get<TransForm>(activeEnt_);

				cameraPos_ = trans.pos;
				cameraPos_ += Vec3f(0, 0, 20.f);
				cameraAngle_ = trans.quat.getEuler();
				cameraAngleDeg_ = trans.quat.getEulerDegrees();
				
			}
			else
			{
				X_ASSERT_UNREACHABLE();
			}
		}

		cam_.set(mat33, cameraPos_);
	//	cam_.setAngles(cameraAngle_);
	//	cam_.setPosition(cameraPos_);

		// Pro
		frame.view.cam = cam_;
		frame.view.projMatrix = cam_.getProjectionMatrix();
		frame.view.viewMatrix = cam_.getViewMatrix();

#if 1
	//	frame.view.viewMatrix = mat33;
	//	frame.view.viewMatrix.setTranslate(cameraPos_);
	//	frame.view.viewMatrix.transpose();
#else

		Matrix44f pickle = Matrix44f::identity();
		Matrix44f trans = Matrix44f::createTranslation(cameraPos_);

		pickle = Matrix44f(mat33) * trans;

		frame.view.viewMatrix = pickle;
		frame.view.viewMatrix.transpose();
#endif

		frame.view.viewProjMatrix = frame.view.viewMatrix * frame.view.projMatrix;
		frame.view.viewProjInvMatrix = frame.view.viewProjMatrix.inverted();

		// for now sound listner follows camera.
		Transformf trans;
		trans.pos = cameraPos_;
		trans.quat = Quatf(mat33);

		gEnv->pSound->setListenPos(trans);
	}

	void CameraSystem::setActiveEnt(EntityId entId)
	{
		activeEnt_ = entId;
	}


} // namespace entity

X_NAMESPACE_END