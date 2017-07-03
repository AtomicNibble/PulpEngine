#include "stdafx.h"
#include "GameVars.h"

#include <IConsole.h>


X_NAMESPACE_BEGIN(game)


GameVars::GameVars()
{

}


void GameVars::registerVars(void)
{
	pFovVar_ = ADD_CVAR_FLOAT("cam_fov", ::toDegrees(DEFAULT_FOV), 0.01f, ::toDegrees(PIf),
		core::VarFlag::SAVE_IF_CHANGED, "camera fov");




}


X_NAMESPACE_END