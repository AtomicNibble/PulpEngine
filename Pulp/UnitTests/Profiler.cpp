#include "stdafx.h"
#include "Profiler.h"


namespace UnitTests
{
	double ScopeProfiler::s_frequency;

	ScopeProfiler::ScopeProfiler(const char* name) : name_(name) 
	{
		start_ = getTime();
	}

	ScopeProfiler::~ScopeProfiler()
	{
		int64_t elapsed = (getTime() - start_);
		double ms = convertToMs(elapsed);

		X_LOG0(name_, "Operation took %5.4fms", ms);
	}


	double ScopeProfiler::convertToMs(int64_t elapsed) 
	{
		if (s_frequency == 0.f)
		{
			LARGE_INTEGER freq;
			QueryPerformanceFrequency(&freq);
			s_frequency = 1.0 / double(freq.QuadPart);
			s_frequency *= 1000.0;
		}

		return elapsed * s_frequency;
	}

	int64_t ScopeProfiler::getTime(void)
	{
		LARGE_INTEGER start;
		QueryPerformanceCounter(&start);
		return  start.QuadPart;
	}

} // namespace UnitTests