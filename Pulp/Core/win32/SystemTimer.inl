

namespace SysTimer
{

	X_INLINE int64_t Get(void)
	{
		typedef int64(*TimeUpdateFunc) ();
		extern TimeUpdateFunc g_pUpdateFunc;

	//	TimeUpdateFunc test = g_pUpdateFunc;
		return g_pUpdateFunc();
	}

	X_INLINE float ToSeconds(int64_t count)
	{
		extern float g_oneOverFrequency;
		return count * g_oneOverFrequency;
	}

	X_INLINE float ToMilliSeconds(int64_t count)
	{
		extern float g_thousandOverFrequency;
		return count * g_thousandOverFrequency;
	}

	X_INLINE int64 GetTickPerSec()
	{
		extern int64_t g_Frequency;
		return g_Frequency;
	}
}
