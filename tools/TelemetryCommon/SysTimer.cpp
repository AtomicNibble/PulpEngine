#include "stdafx.h"
#include "SysTimer.h"

SysTimer::SysTimer()
{
    oneOverFrequency_ = 0.f;
    thousandOverFrequency_ = 0.f;

    frequency_ = 0;
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
    millionOverFrequency_ = static_cast<float>(resolution * 1000000.0);
    frequencyOverMillion_ = frequency.QuadPart / 1000000;
    return true;
}

tt_int64 SysTimer::GetTicks(void)
{
    LARGE_INTEGER now = {};
    QueryPerformanceCounter(&now);
    return static_cast<tt_int64>(now.QuadPart);
}