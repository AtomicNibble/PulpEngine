#pragma once

X_NAMESPACE_BEGIN(core)

namespace profiler
{
    class ProfilerVars
    {
    public:
        ProfilerVars();
        ~ProfilerVars() = default;

        void RegisterVars(void);

        X_INLINE bool isPaused(void) const;
        X_INLINE int32_t getProlfilerDrawFlags(void) const;
        X_INLINE bool drawProfilerConsoleExpanded(void) const;
        X_INLINE int32_t jobSysThreadMS(void) const;

    private:
        int32_t profilerPause_;

        int32_t profilerDrawFlags_;
        int32_t drawProfilerConsoleExpanded_;
        int32_t jobSysThreadMS_;
    };

} // namespace profiler

X_NAMESPACE_END

#include "ProfilerVars.inl"