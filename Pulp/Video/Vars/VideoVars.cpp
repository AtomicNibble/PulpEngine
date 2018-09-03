#include "stdafx.h"
#include "VideoVars.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(video)

VideoVars::VideoVars()
{
    drawDebug_ = 1;
}

VideoVars::~VideoVars()
{

}

void VideoVars::RegisterVars(void)
{
    ADD_CVAR_REF("video_draw_debug", drawDebug_, drawDebug_, 0, 1, core::VarFlag::SYSTEM,
        "Draw video playback debug");

}


X_NAMESPACE_END