#pragma once


X_NAMESPACE_BEGIN(core)

namespace profiler
{

	X_INLINE bool ProfilerVars::isPaused(void) const
	{
		return profilerPause_ != 0;
	}

	X_INLINE bool ProfilerVars::drawProfileInfo(void) const
	{
		return drawProfileInfo_ != 0;
	}


} // namespace profiler

X_NAMESPACE_END