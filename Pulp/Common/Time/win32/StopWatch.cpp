#include "EngineCommon.h"
#include "../StopWatch.h"

#include "Traits\FunctionTraits.h"

#ifndef NEAR
#define NEAR near
#endif

#ifndef FAR
#define FAR far
#endif

X_NAMESPACE_BEGIN(core)

namespace
{
    class SysTimer
    {
        typedef core::traits::Function<int64(void)> TimeUpdateFunc;

    public:
        SysTimer();

    public:
        bool StartUp(void);

        int64_t Get(void);
        float ToSeconds(int64_t count);
        float ToMilliSeconds(int64_t count);

    private:
        float oneOverFrequency_;
        float thousandOverFrequency_;
        int64_t frequency_;
    };

    SysTimer::SysTimer()
    {
        oneOverFrequency_ = 0.f;
        thousandOverFrequency_ = 0.f;

        frequency_ = 0;

        StartUp();
    }

    bool SysTimer::StartUp(void)
    {
        LARGE_INTEGER frequency;

        if (!QueryPerformanceFrequency(&frequency)) {
            return false;
        }

        frequency_ = frequency.QuadPart;

        double resolution = 1.0 / static_cast<double>(frequency.QuadPart);

        oneOverFrequency_ = static_cast<float>(resolution);
        thousandOverFrequency_ = static_cast<float>(resolution * 1000.0);
        return true;
    }

    X_INLINE int64_t SysTimer::Get(void)
    {
        LARGE_INTEGER now = {};
        QueryPerformanceCounter(&now);
        return static_cast<int64_t>(now.QuadPart);
    }

    X_INLINE float SysTimer::ToSeconds(int64_t count)
    {
        return static_cast<float>(count * oneOverFrequency_);
    }

    X_INLINE float SysTimer::ToMilliSeconds(int64_t count)
    {
        return static_cast<float>(count * thousandOverFrequency_);
    }

    SysTimer g_sysTimer;

} // namespace

TimeVal StopWatch::GetTimeNow(void)
{
    return TimeVal(g_sysTimer.Get());
}

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
