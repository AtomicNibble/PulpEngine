#pragma once

#ifndef _X_TIMER_H_
#define _X_TIMER_H_


#include <ITimer.h>

X_NAMESPACE_BEGIN(core)

// set the frame range.
#define FPS_FRAMES 16
#define MAX_FRAME_AVERAGE 100


class XTimer : public ITimer
{
public:
	XTimer();

//	virtual void StartUp();
//	virtual void ShutDown();

	bool Init(ICore *pCore);

	// reset the timers.
	virtual void Reset();

	// start of frame sets the timers values.
	virtual void OnFrameBegin();

	virtual float GetCurrTime(Timer::Enum which = Timer::GAME) {
		return Timers_[which].GetSeconds();
	}

	// returns the start time of the frame for the respective timer
	virtual const TimeVal& GetFrameStartTime(Timer::Enum which = Timer::GAME) {
		return Timers_[which];
	}

	// gets the time right now.
	virtual TimeVal GetAsyncTime() const;
	// gets the time right now as seconds.
	virtual float GetAsyncCurTime();

	// gets avg frame time.
	virtual float GetFrameTime() const;

	// get the time scale
	virtual float GetTimeScale();
	// set hte time scale
	virtual void SetTimeScale(float scale);

	// returns the current frame weight
	virtual float GetFrameRate();

private:
	void RefreshTime(Timer::Enum which, int64 curTime);


	float GetAverageFrameTime(float sec, float FrameTime, float LastAverageFrameTime);

private:
	int64			BaseTime_;		// the time when timer started	
	int64			LastTime_;		// the time from last update, used to calculate time since last frame.

	int64			CurrentTime_;
	int64			TicksPerSec_;

	size_t			FrameCounter_;

	float			FrameTime_;			// scaled 
	float			FrameTimeActual_;	// frame time without scaling
	float           time_scale_;

	float			max_frame_time_;

	int				max_fps_;
	int				debugTime_;

};




X_NAMESPACE_END

#endif // !_X_TIMER_H_
