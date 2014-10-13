#include "stdafx.h"
#include "SystemTimer.h"

#include <Util\LastError.h>


#ifndef NEAR
#define NEAR near
#endif

#ifndef FAR
#define FAR far
#endif

#include "Mmsystem.h"

// needed for timeGetTime
// I add the link here since this file is only used for 
// windows builds.
X_LINK_LIB("Winmm.lib")

X_NAMESPACE_BEGIN(core)

namespace SysTimer
{
	typedef int64(*TimeUpdateFunc) ();
	TimeUpdateFunc g_pUpdateFunc;

	float g_oneOverFrequency; 
	float g_thousandOverFrequency; 

	int64_t g_Frequency;

	namespace
	{
		static int64_t PerformanceCounterTime(void)
		{
			LARGE_INTEGER now = {};
			QueryPerformanceCounter(&now);
			return static_cast<int64_t>(now.QuadPart);
		}

		static int64_t MMTimeGet(void)
		{
			return  static_cast<int64_t>(timeGetTime());
		}
	}

	void Startup(void)
	{
		LARGE_INTEGER frequency;

		if( QueryPerformanceFrequency( &frequency ) )
		{
			g_Frequency = frequency.QuadPart;
			g_pUpdateFunc = &PerformanceCounterTime;
		}
		else
		{
			lastError::Description Dsc;
			X_WARNING("SysTimer", "Failed to query performance timer. Error: ", lastError::ToString(Dsc));

			g_Frequency = 1000;
			g_pUpdateFunc = &MMTimeGet;
		}

		double resolution = 1.0 / static_cast<double>(frequency.QuadPart);

		g_oneOverFrequency = static_cast<float>(resolution);
		g_thousandOverFrequency = static_cast<float>(resolution * 1000.0);
	}

	void Shutdown(void)
	{
		g_oneOverFrequency = 0;
		g_thousandOverFrequency = 0;
	}

}

X_NAMESPACE_END