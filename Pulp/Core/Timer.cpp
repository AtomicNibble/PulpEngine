#include "stdafx.h"
#include "Timer.h"

#include <ICore.h>
#include <IConsole.h>
#include <IFrameData.h>


#include "SystemTimer.h"

//! Profile smoothing time in seconds (original default was .8 / log(10) ~= .35 s)
static const float fDEFAULT_PROFILE_SMOOTHING = 1.0f;
static const float fMAX_BLEND_FRAME_TIME = 1.0f;


X_NAMESPACE_BEGIN(core)

namespace
{

	X_INLINE int64_t scaleTime(int64_t time, float32_t scale)
	{
		const double timeD = static_cast<double>(time);
		const double scaleD = static_cast<double>(scale);

		const double result = timeD * scaleD;

		return static_cast<int64_t>(result);
	}


} // namespace


XTimer::XTimer() :
	timeScale_(1.f),
	timeScaleUi_(1.f),
	maxFrameTimeDelta_(0),
	ticksPerSec_(0),
	maxFps_(0),
	debugTime_(0)
{


}


bool XTimer::Init(ICore* pCore)
{
	X_UNUSED(pCore);
	SysTimer::Startup();

	ticksPerSec_ = SysTimer::GetTickPerSec();
	maxFrameTimeDelta_ = ticksPerSec_ / 5; // 0.2f
	timeScale_ = 1; 
	timeScaleUi_ = 1; 

	Reset();

	ADD_CVAR_REF("time_debug", debugTime_, 0, 0, 1, 0, "Time debugging");
	core::ICVar* pVar = ADD_CVAR_FLOAT("time_max_frametime", 0.20f, 0.f, 10000.f, 0, "max time a frame can take (unscaled)");
	ADD_CVAR_REF("time_scale", timeScale_, 1.f, 0, 2000.f, 0, "scale time of each frame");
	ADD_CVAR_REF("time_scale_ui", timeScaleUi_, 1.f, 0, 2000.f, 0, "scale time of each UI frame");
//	ADD_CVAR_REF("maxfps", maxFps_, 24, 0, 1000, 0, "Max fps 0=unlimated");


	core::ConsoleVarFunc del;
	del.Bind<XTimer, &XTimer::OnMaxFrameTimeChanged>(this);
	pVar->SetOnChangeCallback(del);

	return true; 
}



//////////////////////////////////////////////////


void XTimer::Reset(void)
{
	baseTime_ = SysTimer::Get();
	lastFrameStartTime_ = 0;
	currentTime_ = 0;
}


//////////////////////////////////////////////////

void XTimer::OnFrameBegin(core::FrameTimeData& frameTime)
{
	const int64_t now = SysTimer::Get();

	currentTime_ = now - baseTime_;

	const int64_t frameDelta = currentTime_ - lastFrameStartTime_;
	const int64_t frameDeltaCapped = core::Min(frameDelta, maxFrameTimeDelta_);

	if (frameDelta != frameDeltaCapped && debugTime_)
	{
		const float32_t deltaMs = SysTimer::ToMilliSeconds(frameDelta);
		const float32_t cappedDeltaMs = SysTimer::ToMilliSeconds(frameDeltaCapped);
		X_LOG0("Time", "Frame delta was capped. delta: %f cappedDelta: %f", deltaMs, cappedDeltaMs);
	}

	frameTime.startTimeReal.SetValue(currentTime_);

	// set the unscaled timers
	frameTime.unscaledDeltas[Timer::GAME].SetValue(frameDeltaCapped);
	frameTime.unscaledDeltas[Timer::UI].SetValue(frameDeltaCapped);

	// now we need to scale each of the timers.
	{
		const int64_t gameTime = scaleTime(frameDeltaCapped, timeScale_);
		frameTime.deltas[Timer::GAME].SetValue(frameDeltaCapped);
	}
	{
		const int64_t uiTime = scaleTime(frameDeltaCapped, timeScaleUi_);
		frameTime.deltas[Timer::UI].SetValue(uiTime);
	}

	// when scaling this should work correct since none of the members get scaled only values in frame delta.


	// set last time for use next frame.
	lastFrameStartTime_ = currentTime_;

	if (debugTime_)
	{
		X_LOG0("Timer", "Cur=%" PRId64 " Now=%" PRId64 " Game=%f UI=%f",
			currentTime_,
			now,
			frameTime.deltas[Timer::GAME].GetMilliSeconds(),
			frameTime.deltas[Timer::UI].GetMilliSeconds()
		);
	}
}


TimeVal XTimer::GetTimeNow(Timer::Enum timer) const
{
	X_UNUSED(timer);
	int64 now = SysTimer::Get();
	// TODO add scale.

	return TimeVal(now - baseTime_);
}


TimeVal XTimer::GetTimeNowNoScale(void) const
{
	int64 now = SysTimer::Get();

	return TimeVal(now - baseTime_);
}


TimeVal XTimer::GetTimeNowReal(void) const
{
	int64 now = SysTimer::Get();

	return TimeVal(now);
}


float XTimer::GetAvgFrameTime(void) const
{
	return 0.1f;
}

float XTimer::GetAvgFrameRate(void)
{
	return 0.f;
}

void XTimer::OnMaxFrameTimeChanged(core::ICVar* pVar)
{
	const float val = pVar->GetFloat();

	// turn it into ticks.
	maxFrameTimeDelta_ = static_cast<int64_t>(ticksPerSec_ * val);
}


X_NAMESPACE_END