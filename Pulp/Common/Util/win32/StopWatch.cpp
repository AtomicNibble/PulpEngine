#include "EngineCommon.h"
#include "../StopWatch.h"


#ifndef NEAR
#define NEAR near
#endif

#ifndef FAR
#define FAR far
#endif

#include "Mmsystem.h"
// Winmm.lib is delay loaded.

X_NAMESPACE_BEGIN(core)

namespace
{

	class SysTimer
	{
		typedef int64(*TimeUpdateFunc) ();

	public:
		SysTimer();

	public:
		void StartUp(void);

		int64_t Get(void);
		float ToSeconds(int64_t count);
		float ToMilliSeconds(int64_t count);

	private:
		static int64_t PerformanceCounterTime(void);
		static int64_t MMTimeGet(void);

	private:
		TimeUpdateFunc pUpdateFunc_;

		float oneOverFrequency_;
		float thousandOverFrequency_;
		int64_t frequency_;
	};

	SysTimer::SysTimer()
	{
		pUpdateFunc_ = nullptr;
		oneOverFrequency_ = 0.f;
		thousandOverFrequency_ = 0.f;

		frequency_ = 0;

		StartUp();
	}

	void SysTimer::StartUp(void)
	{
		LARGE_INTEGER frequency;

		if (QueryPerformanceFrequency(&frequency))
		{
			frequency_ = frequency.QuadPart;
			pUpdateFunc_ = &PerformanceCounterTime;
		}
		else
		{
			frequency_ = 1000;
			pUpdateFunc_ = &MMTimeGet;
		}

		pUpdateFunc_ = &MMTimeGet;

		double resolution = 1.0 / static_cast<double>(frequency.QuadPart);

		oneOverFrequency_ = static_cast<float>(resolution);
		thousandOverFrequency_ = static_cast<float>(resolution * 1000.0);
	}

	int64_t SysTimer::Get(void)
	{
		return pUpdateFunc_();
	}

	float SysTimer::ToSeconds(int64_t count)
	{
		return static_cast<float>(count * oneOverFrequency_);
	}


	float SysTimer::ToMilliSeconds(int64_t count)
	{
		return static_cast<float>(count * thousandOverFrequency_);
	}

	// ===================================================

	int64_t SysTimer::PerformanceCounterTime(void)
	{
		LARGE_INTEGER now = {};
		QueryPerformanceCounter(&now);
		return static_cast<int64_t>(now.QuadPart);
	}

	int64_t SysTimer::MMTimeGet(void)
	{
		return static_cast<int64_t>(timeGetTime());
	}


	SysTimer g_sysTimer;

} // namespace


int64_t StopWatch::SysGet(void)
{
	return g_sysTimer.Get();
}

float StopWatch::SysToSeconds(int64_t count)
{
	return g_sysTimer.ToSeconds(count);
}

float StopWatch::SysToMilliSeconds(int64_t count)
{
	return g_sysTimer.ToMilliSeconds(count);
}


X_NAMESPACE_END