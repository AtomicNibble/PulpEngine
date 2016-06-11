#pragma once

#include <Time\TimeVal.h>


X_NAMESPACE_BEGIN(core)

X_DECLARE_FLAGS(FrameFlag)(
	HAS_FOCUS

);


struct FrameData
{
	typedef Flags<FrameFlag> FrameFlags;

	TimeVal startTime;
	float delta;

	FrameFlags flags;

};


X_NAMESPACE_END