#include <EngineCommon.h>
#include "CompressedStamps.h"

X_NAMESPACE_BEGIN(core)

DateStampSmall DateStampSmall::systemDate(void)
{
    _SYSTEMTIME time;
    GetLocalTime(&time);

    return DateStampSmall(time.wYear, time.wMonth, time.wDay);
}

TimeStampSmall TimeStampSmall::systemTime(void)
{
    _SYSTEMTIME time;
    GetLocalTime(&time);

    return TimeStampSmall(time.wHour, time.wMinute, time.wSecond);
}

DateTimeStampSmall DateTimeStampSmall::systemDateTime(void)
{
    _SYSTEMTIME time;
    GetLocalTime(&time);

    return DateTimeStampSmall(time.wYear, time.wMonth, time.wDay,
        time.wHour, time.wMinute, time.wSecond);
}

X_NAMESPACE_END