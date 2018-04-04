#pragma once

#ifndef _X_TIMER_I_H_
#define _X_TIMER_I_H_

#include <Time\TimeVal.h>
#include <Util\FlagsMacros.h>


X_NAMESPACE_DECLARE(core,
	struct FrameTimeData;
);

X_NAMESPACE_BEGIN(core)

X_DECLARE_ENUM(Timer)(
	GAME,
	UI
);

struct ITimer
{
	typedef Timer Timer;

	virtual ~ITimer() = default;

	virtual bool init(ICore* pCore) X_ABSTRACT;
	virtual void reset(void) X_ABSTRACT;


	// start of frame sets the timers values.
	virtual void OnFrameBegin(core::FrameTimeData& frameTime) X_ABSTRACT;


	// returns the time now relative to base time, no scaling
	virtual TimeVal GetTimeNowNoScale(void) const X_ABSTRACT;

	// returns the absolute time right now, not delta from base time.
	// do i even have use for this, since all these are time stamps.
	virtual TimeVal GetTimeNowReal(void) const X_ABSTRACT;

	virtual TimeVal GetAvgFrameTime(void) const X_ABSTRACT;
	virtual float GetAvgFrameRate(void) X_ABSTRACT;

	virtual float GetScale(Timer::Enum timer) X_ABSTRACT;


};

X_NAMESPACE_END

#endif // !_X_TIMER_I_H_
