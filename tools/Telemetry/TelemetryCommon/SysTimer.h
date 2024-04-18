#pragma once

#include "Types.h"

class SysTimer
{
public:
    TELEMETRY_COMLIB_EXPORT SysTimer();
    TELEMETRY_COMLIB_EXPORT bool StartUp(void);

    TELEMETRY_COMLIB_EXPORT tt_int64 GetTicks(void);
    TELEM_INLINE tt_int64 GetNano(void);
    TELEM_INLINE tt_int64 GetMicro(void);
    TELEM_INLINE float ToSeconds(tt_int64 count);
    TELEM_INLINE float ToMilliSeconds(tt_int64 count);

private:
    float oneOverFrequency_;
    float thousandOverFrequency_;
    float millionOverFrequency_;
    tt_int64 frequencyOverMillion_;
    tt_int64 frequency_;
};


TELEM_INLINE tt_int64 SysTimer::GetNano(void)
{
    const auto count = GetTicks();
    const tt_int64 whole = (count / frequency_) * 1000000000;
    const tt_int64 part = (count % frequency_) * 1000000000 / frequency_;

    return whole + part;
}

TELEM_INLINE tt_int64 SysTimer::GetMicro(void)
{
    const auto count = GetTicks();
    const tt_int64 whole = (count / frequency_) * 1000000;
    const tt_int64 part = (count % frequency_) * 1000000 / frequency_;

    return whole + part;
}

TELEM_INLINE float SysTimer::ToSeconds(tt_int64 count)
{
    return static_cast<float>(count * oneOverFrequency_);
}

TELEM_INLINE float SysTimer::ToMilliSeconds(tt_int64 count)
{
    return static_cast<float>(count * thousandOverFrequency_);
}
