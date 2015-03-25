#pragma once

#ifndef X_UT_PROFILER_H_
#define X_UT_PROFILER_H_

// profiler just for use inside unit tests.
namespace UnitTests
{

class ScopeProfiler
{
public:
	ScopeProfiler(const char* name);
	~ScopeProfiler();

private:
	const char* name_;
	int64_t start_;

	static double s_frequency;

public:
	static double convertToMs(int64_t elapsed);
	static int64_t getTime(void);
};


} // namespace UnitTests

#endif // !X_UT_PROFILER_H_