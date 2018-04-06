#pragma once

X_NAMESPACE_BEGIN(core)

namespace profiler
{
    X_INLINE bool ProfilerVars::isPaused(void) const
    {
        return profilerPause_ != 0;
    }

    X_INLINE int32_t ProfilerVars::getProlfilerDrawFlags(void) const
    {
        return profilerDrawFlags_;
    }

    X_INLINE bool ProfilerVars::drawProfilerConsoleExpanded(void) const
    {
        return drawProfilerConsoleExpanded_ != 0;
    }

    X_INLINE int32_t ProfilerVars::jobSysThreadMS(void) const
    {
        return jobSysThreadMS_;
    }

} // namespace profiler

X_NAMESPACE_END