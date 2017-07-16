#include "stdafx.h"
#include "GameVars.h"

#include <IConsole.h>


X_NAMESPACE_BEGIN(game)


GameVars::GameVars()
{
	
	cameraPos_ = Vec3f(0, -150, 150);
	cameraAngle_ = Vec3f(toRadians(-45.f), 0, toRadians(0.f));

}


void GameVars::registerVars(void)
{
	pFovVar_ = ADD_CVAR_FLOAT("cam_fov", ::toDegrees(DEFAULT_FOV), 0.01f, ::toDegrees(PIf),
		core::VarFlag::SAVE_IF_CHANGED, "camera fov");



}


X_NAMESPACE_END