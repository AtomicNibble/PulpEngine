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


XTimer::XTimer() :
	timeScale_(1),
	timeScaleUi_(1),
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
	maxFrameTimeDelta_ = ticksPerSec_ / 5; // 0.2

	Reset();

	ADD_CVAR_REF("time_debug", debugTime_, 0, 0, 1, 0, "Time debugging");
//	ADD_CVAR_REF("time_max_frametime", max_frame_time_, 0.20f, 0.f, 10000.f, 0, "max time a frame can take");
//	ADD_CVAR_REF("time_scale", timeScale_, 1.f, 0, 2000.f, 0, "scale time of each frame");
//	ADD_CVAR_REF("time_scale_ui", timeScaleUi_, 1.f, 0, 2000.f, 0, "scale time of each UI frame");
//	ADD_CVAR_REF("maxfps", maxFps_, 24, 0, 1000, 0, "Max fps 0=unlimated");

	return true; 
}



//////////////////////////////////////////////////


void XTimer::Reset(void)
{
	baseTime_ = SysTimer::Get();
	lastFrameStartTime_ = 0;
	currentTime_ = 0;

//	RefreshTime(Timer::GAME, 0);
//	RefreshTime(Timer::UI, 0);
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
		const int64_t gameTime = frameDeltaCapped / timeScale_;
		frameTime.deltas[Timer::GAME].SetValue(frameDeltaCapped);
	}
	{
		const int64_t uiTime = frameDeltaCapped / timeScaleUi_;
		frameTime.deltas[Timer::UI].SetValue(uiTime);
	}


	// set last time for use next frame.
	lastFrameStartTime_ = currentTime_;

#if 0
	int64 now = SysTimer::Get();

	// time since base.
	CurrentTime_ = now - BaseTime_;

	// work out the time last frame took
	FrameTime_ = FrameTimeActual_ = (float)((double)(CurrentTime_ - LastTime_) / (double)(TicksPerSec_));

	if (max_fps_ != 0) // 0 is unlimated
	{	
		X_PROFILE_BEGIN("FpsCapSleep", core::ProfileSubSys::UNCLASSIFIED);

		float TargetFrameTime = (1.f / (float)max_fps_);
		float sleep = TargetFrameTime - FrameTime_;
		if (sleep > 0)
		{
			core::Thread::Sleep((uint32_t)(sleep * 1000.f)); // scale to ms
	
			CurrentTime_ = SysTimer::Get() - BaseTime_;
			// update actual frame time.
			FrameTimeActual_ = (float)((double)(CurrentTime_ - LastTime_) / (double)(TicksPerSec_));
		}

		FrameTime_ = abs(TargetFrameTime);
	}
	// enable scale when max fps is on.
	// else
	{
		// Clamp it
		FrameTime_ = core::Min(FrameTime_, max_frame_time_);

		// scale it
		FrameTime_ *= time_scale_;
	}

	// make sure it's not negative
	if (FrameTime_ < 0)
		FrameTime_ = 0;


	// if time is scaled etc..
	if (FrameTime_ != FrameTimeActual_)
	{
		int64 nAdjust = (int64)((FrameTime_ - FrameTimeActual_) * (double)(TicksPerSec_));
		CurrentTime_ += nAdjust;
		BaseTime_ -= nAdjust;
	}


	// refresh the timers.
	RefreshTime(Timer::GAME, CurrentTime_);
	RefreshTime(Timer::UI, CurrentTime_); // don't scale?

	if (FrameTimeActual_ < 0.f)
		FrameTimeActual_ = 0.0f;


	// set last time for use next frame.
	LastTime_ = CurrentTime_;


	if (debugTime_)
	{
		X_LOG0("Timer", "Cur=%" PRId64 " Now=%" PRId64 " Async=%f CurrTime=%f UI=%f",
			CurrentTime_,
			now, 
			GetAsyncCurTime(), 
			GetCurrTime(Timer::GAME),
			GetCurrTime(Timer::UI)
			);
	}

#endif
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


///////////////////////////////////////////////////

// updates the timer, 
void XTimer::RefreshTime(Timer::Enum which, int64 curTime)
{
	X_UNUSED(which);
	X_UNUSED(curTime);

//	Timers_[which].SetValue(curTime);
}





X_NAMESPACE_END