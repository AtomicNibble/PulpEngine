#pragma once

#include <Time\TimeVal.h>

#include <IInput.h>
#include <ITimer.h>

X_NAMESPACE_BEGIN(core)

X_DECLARE_FLAGS(FrameFlag)(
	HAS_FOCUS

);


struct FrameInput
{
	static const size_t MAX_INPUT_EVENTS_PER_FRAME = 256;

	core::FixedArray<input::InputEvent, MAX_INPUT_EVENTS_PER_FRAME> events;

};


struct FrameTimeData
{
	TimeVal startTimeReal; // unscaled relative time to start of timer.

	TimeVal deltas[ITimer::Timer::ENUM_COUNT];
	TimeVal unscaledDeltas[ITimer::Timer::ENUM_COUNT];
};


struct FrameData
{
	typedef Flags<FrameFlag> FrameFlags;

	FrameTimeData timeInfo;

	FrameFlags flags;

	FrameInput input;
};


X_NAMESPACE_END