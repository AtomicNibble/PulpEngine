#include "stdafx.h"
#include "CoreVars.h"

#include "Platform\Window.h"

#include <IConsole.h>
#include <Threading\JobSystem2.h>

X_NAMESPACE_BEGIN(core)

CoreVars::CoreVars() :
    pWinPosX_(nullptr),
    pWinPosY_(nullptr),
    pWinWidth_(nullptr),
    pWinHeight_(nullptr),
    pWinCustomFrame_(nullptr)
{
    // defaults.
    schedulerNumThreads_ = 0;
    coreFastShutdown_ = 0;
    coreEventDebug_ = 0;

    winXPos_ = 10;
    winYPos_ = 10;
    winWidth_ = 800;
    winHeight_ = 600;
}

void CoreVars::registerVars(void)
{
    ADD_CVAR_REF("core_scheduler_threads", schedulerNumThreads_, schedulerNumThreads_, 0, core::V2::JobSystem::HW_THREAD_MAX, VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED,
        "Number of threads to create for scheduler. 0=auto");

    ADD_CVAR_REF("core_fast_shutdown", coreFastShutdown_, coreFastShutdown_, 0, 1, VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED,
        "Skips most cleanup logic for faster shutdown, when off everything is correctly shutdown and released before exit. 0=off 1=on");
    ADD_CVAR_REF("core_event_debug", coreEventDebug_, coreEventDebug_, 0, 1, VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED,
        "Debug messages for core events. 0=off 1=on");

    core::xWindow::Rect desktop = core::xWindow::GetDesktopRect();

    pWinPosX_ = ADD_CVAR_REF("win_x_pos", winXPos_, winXPos_, 0, desktop.getWidth(),
        VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED, "Game window position x");
    pWinPosY_ = ADD_CVAR_REF("win_y_pos", winYPos_, winYPos_, 0, desktop.getHeight(),
        VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED, "Game window position y");
    pWinWidth_ = ADD_CVAR_REF("win_width", winWidth_, winWidth_, 800, 1,
        VarFlag::SYSTEM, "Game window width");
    pWinHeight_ = ADD_CVAR_REF("win_height", winHeight_, winHeight_, 600, 1,
        VarFlag::SYSTEM, "Game window height");

    pWinCustomFrame_ = ADD_CVAR_INT("win_custom_Frame", 1, 0, 1,
        VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED, "Enable / disable the windows custom frame");

    const char* pVersionStr = (X_ENGINE_NAME "Engine " X_PLATFORM_STR "-" X_CPUSTRING " Version " X_ENGINE_VERSION_STR);
    ADD_CVAR_STRING("version", pVersionStr,
        VarFlag::SYSTEM | VarFlag::READONLY, "Engine Version");
    ADD_CVAR_STRING("build_ref", X_STRINGIZE(X_ENGINE_BUILD_REF),
        VarFlag::SYSTEM | VarFlag::READONLY, "Engine Version");
    ADD_CVAR_STRING("build_date", X_ENGINE_BUILD_DATE,
        VarFlag::SYSTEM | VarFlag::READONLY, "Engine Build Date");
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

int32_t CoreVars::getWinPosX(void) const
{
    return (X_ASSERT_NOT_NULL(pWinPosX_))->GetInteger();
}

int32_t CoreVars::getWinPosY(void) const
{
    return (X_ASSERT_NOT_NULL(pWinPosY_))->GetInteger();
}

int32_t CoreVars::getWinWidth(void) const
{
    return (X_ASSERT_NOT_NULL(pWinWidth_))->GetInteger();
}

int32_t CoreVars::getWinHeight(void) const
{
    return (X_ASSERT_NOT_NULL(pWinHeight_))->GetInteger();
}

X_NAMESPACE_END