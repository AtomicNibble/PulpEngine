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
	~XTimer() X_OVERRIDE = default;

	bool Init(ICore *pCore) X_FINAL;

	// reset the timers, so delta from start time is zero.
	void Reset(void) X_FINAL;

	void OnFrameBegin(core::FrameTimeData& frameTime) X_FINAL;

	// returns the absolute time right now, not delta from base time.
	TimeVal GetTimeNow(Timer::Enum timer = Timer::GAME) const X_FINAL;
	TimeVal GetTimeNowNoScale(void) const X_FINAL;

	// returns the time now relative to base time.
	TimeVal GetTimeNowReal(void) const X_FINAL;

	float GetAvgFrameTime(void) const X_FINAL;
	float GetAvgFrameRate(void) X_FINAL;


private:
	void RefreshTime(Timer::Enum which, int64 curTime);

	float GetAverageFrameTime(float sec, float FrameTime, float LastAverageFrameTime);

private:
	int64_t baseTime_;				// time we started / reset
	int64_t lastFrameStartTime_;	// start time relative to base time
	int64_t currentTime_;			// relative to base.

	int64_t timeScale_;				// scale for game
	int64_t timeScaleUi_;			// scale for ui	  
	int64_t maxFrameTimeDelta_;		// cap delta's that exceed this value.
	int64_t ticksPerSec_;

	int32_t	debugTime_;
	int32_t	maxFps_;
};




X_NAMESPACE_END

#endif // !_X_TIMER_H_
