#include "stdafx.h"
#include "CoreVars.h"

#include "Platform\Window.h"

#include <IConsole.h>


X_NAMESPACE_BEGIN(core)

CoreVars::CoreVars() :
	pWinPosX_(nullptr),
	pWinPosY_(nullptr),
	pWinWidth_(nullptr),
	pWinHeight_(nullptr),
	pWinCustomFrame_(nullptr)
{

}

void CoreVars::registerVars(void)
{
	ADD_CVAR_REF("core_fast_shutdown", coreFastShutdown_, 0, 0, 1, VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED,
		"Skips most cleanup logic for faster shutdown, when off everything is correctly shutdown and released before exit. 0=off 1=on");
	ADD_CVAR_REF("core_event_debug", coreEventDebug_, 0, 0, 1, VarFlag::SYSTEM,
		"Debug messages for core events. 0=off 1=on");

	core::xWindow::Rect desktop = core::xWindow::GetDesktopRect();

	pWinPosX_ = ADD_CVAR_REF("win_x_pos", winXPos_, 10, 0, desktop.getWidth(),
		VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED, "Game window position x");
	pWinPosY_ = ADD_CVAR_REF("win_y_pos", winYPos_, 10, 0, desktop.getHeight(),
		VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED, "Game window position y");
	pWinWidth_ = ADD_CVAR_REF("win_width", winWidth_, 800, 800, 1,
		VarFlag::SYSTEM, "Game window width");
	pWinHeight_ = ADD_CVAR_REF("win_height", winHeight_, 600, 600, 1,
		VarFlag::SYSTEM, "Game window height");

#if 0
	core::ConsoleVarFunc del;
	del.Bind<XCore, &XCore::WindowPosVarChange>(this);
	var_win_pos_x->SetOnChangeCallback(del);

	del.Bind<XCore, &XCore::WindowPosVarChange>(this);
	var_win_pos_y->SetOnChangeCallback(del);

	del.Bind<XCore, &XCore::WindowSizeVarChange>(this);
	var_win_width->SetOnChangeCallback(del);

	del.Bind<XCore, &XCore::WindowSizeVarChange>(this);
	var_win_height->SetOnChangeCallback(del);

	del.Bind<XCore, &XCore::WindowCustomFrameVarChange>(this);
	var_win_custom_Frame->SetOnChangeCallback(del);



#endif

	pWinCustomFrame_ = ADD_CVAR_INT("win_custom_Frame", 1, 0, 1,
		VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED, "Enable / disable the windows custom frame");


	const char* pVersionStr = (X_ENGINE_NAME "Engine " X_PLATFORM_STR "-" X_CPUSTRING " Version " X_ENGINE_VERSION_STR);
	ADD_CVAR_STRING("version", pVersionStr,
		VarFlag::SYSTEM | VarFlag::READONLY, "Engine Version");
	ADD_CVAR_STRING("build_ref", X_STRINGIZE(X_ENGINE_BUILD_REF), 
		VarFlag::SYSTEM | VarFlag::READONLY, "Engine Version");


}

void CoreVars::updateWinPos(int32_t x, int32_t y)
{
	pWinPosX_->Set(x);
	pWinPosY_->Set(y);
}

void CoreVars::updateWinDim(int32_t width, int32_t height)
{
	pWinWidth_->Set(width);
	pWinHeight_->Set(height);
}



X_NAMESPACE_END