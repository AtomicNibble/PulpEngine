#pragma once

#ifndef _X_TIMER_I_H_
#define _X_TIMER_I_H_

#include "TimeVal.h"

#include <Util\FlagsMacros.h>

X_NAMESPACE_BEGIN(core)

struct ITimer
{
	X_DECLARE_FLAGS(Timer)(GAME, UI);

	virtual ~ITimer() {};

//	virtual void StartUp() X_ABSTRACT;
//	virtual void ShutDown() X_ABSTRACT;

	virtual bool Init(ICore *pCore) X_ABSTRACT;

	// start of frame sets the timers values.
	virtual void OnFrameBegin() X_ABSTRACT;

	// returns the start time of the frame for the respective timer
	virtual float GetCurrTime(Timer::Enum which = Timer::GAME) X_ABSTRACT;

	// returns the start time of the frame for the respective timer
	virtual const TimeVal& GetFrameStartTime(Timer::Enum which = Timer::GAME) X_ABSTRACT;

	// returns the absolute time right now
	virtual TimeVal GetAsyncTime() const X_ABSTRACT;
	// returns the absolute time as seconds
	virtual float GetAsyncCurTime() X_ABSTRACT;

	// gets avg frame time.
	virtual float GetFrameTime() const X_ABSTRACT;

	// Returns the time scale applied to time values.
	virtual float GetTimeScale() X_ABSTRACT;

	// Sets the time scale applied to time values.
	virtual void SetTimeScale(float scale) X_ABSTRACT;

	// Returns the current fps.
	virtual float GetFrameRate() X_ABSTRACT;


protected:

	TimeVal Timers_[Timer::FLAGS_COUNT + 1];

};

X_NAMESPACE_END

#endif // !_X_TIMER_I_H_
