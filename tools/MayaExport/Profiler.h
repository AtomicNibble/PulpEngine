#pragma once

#include <Time\StopWatch.h>
#include <Prepro\PreproUniqueName.h>

class MayaProfiler
{
public:
    MayaProfiler(const char* name);
    ~MayaProfiler();

private:
    const char* name_;
    core::StopWatch timer_;
};

#define PROFILE_MAYA() \
    MayaProfiler X_PP_UNIQUE_NAME(___profile_scope__)(nullptr);

#define PROFILE_MAYA_NAME(name) \
    MayaProfiler X_PP_UNIQUE_NAME(___profile_scope__)(name);