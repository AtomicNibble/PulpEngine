#pragma once

#include <Time\StopWatch.h>

class MayaProfiler
{
public:
	MayaProfiler(const char* name);
	~MayaProfiler();

private:
	const char* name_;
	core::StopWatch timer_;
};

#define PROFILE_MAYA(name) \
	MayaProfiler ___profile_scope__(nullptr);

#define PROFILE_MAYA_NAME(name) \
	MayaProfiler ___profile_scope__(name);