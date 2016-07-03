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


struct FrameView
{

	X_DISABLE_WARNING(4324)

	XCamera cam; // camera pos for this frame.

	X_ALIGN16_MATRIX44F(viewMatrix);
	X_ALIGN16_MATRIX44F(projMatrix);
	X_ALIGN16_MATRIX44F(viewProjMatrix);
	X_ALIGN16_MATRIX44F(viewProjInvMatrix);

	X_ENABLE_WARNING(4324)
};


struct FrameData
{
	typedef Flags<FrameFlag> FrameFlags;

	FrameTimeData timeInfo;
	FrameFlags flags;
	FrameInput input;

	FrameView view;
};


X_NAMESPACE_END