#include "stdafx.h"
#include "Timer.h"

#include <ICore.h>
#include <IConsole.h>

#include <math.h> // TEMP!!

#include <Time\SystemTimer.h>

//! Profile smoothing time in seconds (original default was .8 / log(10) ~= .35 s)
static const float fDEFAULT_PROFILE_SMOOTHING = 1.0f;
static const float fMAX_BLEND_FRAME_TIME = 1.0f;


X_NAMESPACE_BEGIN(core)


XTimer::XTimer() :
time_scale_(1.0f),
FrameCounter_(0),
TicksPerSec_(0),
max_fps_(0),
debugTime_(0)
{
//	AvgFrameTime_ = 1.0f / 60.0f;
//	for (int i = 0; i<MAX_FRAME_AVERAGE; i++)
//		arrFrameTimes_[i] = AvgFrameTime_;

//	smooth_time_ = fDEFAULT_PROFILE_SMOOTHING;
}


bool XTimer::Init(ICore* pCore)
{
	SysTimer::Startup();

	TicksPerSec_ = SysTimer::GetTickPerSec();
	Reset();

	ADD_CVAR_REF("time_debug", debugTime_, 0, 0, 1, 0, "Time debugging");
	ADD_CVAR_REF("time_max_frametime", max_frame_time_, 0.20f, 0.f, 10000.f, 0, "max time a frame can take");
	ADD_CVAR_REF("time_scale", time_scale_, 1.f, 0, 2000.f, 0, "scale time of each frame");
	ADD_CVAR_REF("maxfps", max_fps_, 24, 0, 1000, 0, "Max fps 0=unlimated");

	return true; 
}



//////////////////////////////////////////////////


void XTimer::Reset()
{
	BaseTime_ = SysTimer::Get();
	LastTime_ = 0;
	CurrentTime_ = 0;

	RefreshTime(Timer::GAME, CurrentTime_);
	RefreshTime(Timer::UI, CurrentTime_);
}


//////////////////////////////////////////////////

void XTimer::OnFrameBegin()
{
	X_PROFILE_BEGIN("GameTimer", core::ProfileSubSys::CORE);

	// inc the counter
	FrameCounter_++;

	int64 now = SysTimer::Get();

	// time since base.
	CurrentTime_ = now - BaseTime_;

	// work out the time last frame took
	FrameTime_ = FrameTimeActual_ = (float)((double)(CurrentTime_ - LastTime_) / (double)(TicksPerSec_));

	if (max_fps_ != 0) // 0 is unlimated
	{
		float TargetFrameTime = (1.f / (float)max_fps_);
		float sleep = TargetFrameTime - FrameTime_;
		if (sleep > 0)
		{
			GoatSleep((uint32_t)(sleep * 1000.f)); // scale to ms
	
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
		float fBefore = GetAsyncCurTime();
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
		X_LOG0("Timer", "Cur=%lld Now=%lld Async=%f CurrTime=%f UI=%f",
			CurrentTime_,
			now, 
			GetAsyncCurTime(), 
			GetCurrTime(Timer::GAME),
			GetCurrTime(Timer::UI)
			);

	}
}


TimeVal XTimer::GetAsyncTime() const
{
	int64 now = SysTimer::Get();

	return TimeVal(now);
}

float XTimer::GetAsyncCurTime()
{
	int64 now = SysTimer::Get();

	// turn it into seconds
	double dtime = (double)now;
	float ftime = (float)(dtime / (double)this->TicksPerSec_);

	return ftime;
}


float XTimer::GetFrameTime() const
{
	return FrameTimeActual_;
}


float XTimer::GetTimeScale()
{
	return this->time_scale_;
}

void XTimer::SetTimeScale(float scale)
{
	this->time_scale_ = scale;
}


// we should avg this.
float XTimer::GetFrameRate()
{
	// the frame rate is 1 / the frame time
	if (FrameTimeActual_ != 0.f) // prevent devide by zero.
		return 1.f / FrameTimeActual_;
	return 0.f;
}


///////////////////////////////////////////////////

// updates the timer, 
void XTimer::RefreshTime(Timer::Enum which, int64 curTime)
{
	double dVal = (double)(curTime);

	Timers_[which].SetSeconds((float)(dVal / (double)(TicksPerSec_)));
}





X_NAMESPACE_END