#include "stdafx.h"
#include "CameraSys.h"
#include <IRender.h> // temp

#include <IFrameData.h>
#include <IConsole.h>
#include "Vars\GameVars.h"

X_NAMESPACE_BEGIN(game)

namespace entity
{
    CameraSystem::CameraSystem(GameVars& vars) :
        vars_(vars),
        activeEnt_(EnitiyRegister::INVALID_ID)
    {
    }

    CameraSystem::~CameraSystem()
    {
        if (vars_.getFovVar()) {
            vars_.getFovVar()->SetOnChangeCallback(core::ConsoleVarFunc());
        }

        auto* pConsole = gEnv->pConsole;
        pConsole->UnregisterVariable("cam_pos");
        pConsole->UnregisterVariable("cam_angle");
        pConsole->UnregisterVariable("cam_angle_rad");
    }

    bool CameraSystem::init(void)
    {
        ADD_CVAR_REF_VEC3("cam_pos", cameraPos_, cameraPos_, core::VarFlag::CHEAT | core::VarFlag::READONLY,
            "camera position");
        ADD_CVAR_REF_VEC3("cam_angle", cameraAngleDeg_, cameraAngleDeg_, core::VarFlag::CHEAT | core::VarFlag::READONLY,
            "camera angle(radians)");
        ADD_CVAR_REF_VEC3("cam_angle_rad", cameraAngle_, cameraAngle_, core::VarFlag::CHEAT | core::VarFlag::READONLY,
                "camera angle(radians)");

        auto deimension = gEnv->pRender->getDisplayRes();
        cam_.setFrustum(deimension.x, deimension.y, DEFAULT_FOV, 1.f, 2048.f);

        // we don't store fov var here, since this system only exsistins in game.
        // so can't change the setting out of game.
        if (vars_.getFovVar()) {
            auto* pFov = vars_.getFovVar();

            core::ConsoleVarFunc del;
            del.Bind<CameraSystem, &CameraSystem::OnFovChanged>(this);
            pFov->SetOnChangeCallback(del);
        }

        return true;
    }

    void CameraSystem::OnFovChanged(core::ICVar* pVar)
    {
        float fovDegress = pVar->GetFloat();
        float fov = ::toRadians(fovDegress);

        cam_.setFov(fov);
    }

    void CameraSystem::update(core::FrameData& frame, EnitiyRegister& reg, physics::IScene* pPhysScene)
    {
        X_UNUSED(pPhysScene);

        Matrix33f mat33;

        if (reg.isValid(activeEnt_)) {
            if (reg.has<Player>(activeEnt_)) {
                auto& player = reg.get<Player>(activeEnt_);

                cameraPos_ = player.firstPersonViewOrigin;
                mat33 = player.firstPersonViewAxis;
            }
            else if (reg.has<TransForm>(activeEnt_)) {
                auto& trans = reg.get<TransForm>(activeEnt_);

                cameraPos_ = trans.pos;
                cameraPos_ += Vec3f(0, 0, 20.f);
                cameraAngle_ = trans.quat.getEuler();
                cameraAngleDeg_ = trans.quat.getEulerDegrees();
            }
            else {
                X_ASSERT_UNREACHABLE();
            }
        }

        cam_.set(mat33, cameraPos_);

        frame.view.cam = cam_;
        frame.view.projMatrix = cam_.getProjectionMatrix();
        frame.view.viewMatrix = cam_.getViewMatrix();
        frame.view.viewMatrixInv = frame.view.viewMatrix.inverted();
        // frame.view.viewProjMatrix = frame.view.viewMatrix * frame.view.projMatrix;
        frame.view.viewProjMatrix = frame.view.projMatrix * frame.view.viewMatrix;
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