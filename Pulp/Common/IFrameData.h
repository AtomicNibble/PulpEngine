#pragma once

#include <Time\TimeVal.h>

#include <IInput.h>

X_NAMESPACE_BEGIN(core)

X_DECLARE_FLAGS(FrameFlag)(
	HAS_FOCUS

);


struct FrameInput
{
	static const size_t MAX_INPUT_EVENTS_PER_FRAME = 256;

	core::FixedArray<input::InputEvent, MAX_INPUT_EVENTS_PER_FRAME> events;

};


struct FrameData
{
	typedef Flags<FrameFlag> FrameFlags;

	TimeVal startTime;
	float delta;

	FrameFlags flags;

	FrameInput input;
};


X_NAMESPACE_END