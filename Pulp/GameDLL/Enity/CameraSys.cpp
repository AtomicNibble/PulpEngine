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


		auto* pPosVar = ADD_CVAR_REF_VEC3("cam_pos", cameraPos_, cameraPos_, core::VarFlag::CHEAT,
			"camera position");
		auto* pAngVar = ADD_CVAR_REF_VEC3("cam_angle", cameraAngle_, cameraAngle_, core::VarFlag::CHEAT,
			"camera angle(radians)");


		core::ConsoleVarFunc del;
		del.Bind<CameraSystem, &CameraSystem::OnCamPosChanged>(this);
		pPosVar->SetOnChangeCallback(del);
		del.Bind<CameraSystem, &CameraSystem::OnCamAngChanged>(this);
		pAngVar->SetOnChangeCallback(del);


		return true;
	}

	void CameraSystem::update(core::FrameData& frame, EnitiyRegister& reg, physics::IScene* pPhysScene)
	{
		if (reg.isValid(activeEnt_))
		{
			if (reg.has<TransForm>(activeEnt_))
			{
				auto& trans = reg.get<TransForm>(activeEnt_);

				if (setCamPos_)
				{
					setCamPos_ = false;

					if (reg.has<CharacterController>(activeEnt_))
					{
						physics::ScopedLock lock(pPhysScene, true);

						auto& con = reg.get<CharacterController>(activeEnt_);
						con.pController->setPosition(Vec3d(cameraPos_));
					}
					else
					{
						trans.trans.pos = cameraPos_;
					}
				}
				else
				{
					cameraPos_ = trans.trans.pos;
					cameraPos_ += Vec3f(0, 0, 20.f);
				}

				if (setCamAngle_)
				{
					setCamAngle_ = false;
					trans.trans.quat = Quatf(::toRadians(cameraAngle_.z), toRadians(cameraAngle_.y), toRadians(cameraAngle_.x));

					cameraAngle_ = trans.trans.quat.getEuler();
				}
				else
				{
					cameraAngle_ = trans.trans.quat.getEuler();
				}
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

		// cameraAngle_.x = 0;
	//	cameraAngle_.y = 0;
		// cameraAngle_.z = 0;

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



	void CameraSystem::OnCamPosChanged(core::ICVar* )
	{
		// we want to force camera pos.
		// but we just follow the ent.
		// and the ent might not move in a relative fashion
		// so editing ht ents pos might be pointless.
		// the only way to really move the ent is to cry.



		setCamPos_ = true;
	}

	void CameraSystem::OnCamAngChanged(core::ICVar* )
	{
		setCamAngle_ = true;

	}


} // namespace entity

X_NAMESPACE_END