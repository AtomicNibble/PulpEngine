#include "EngineCommon.h"

#include "TimeStamp.h"

#include <stdio.h>

X_NAMESPACE_BEGIN(core)

// Macro to ensure internal representation is as mill-seconds past midnight
#define REPASMSPM(obj)              \
    do {                            \
        if ((obj).Time_ < 0)        \
            (obj).InternalToMSPM(); \
    }                               \
    X_DISABLE_WARNING(4127)         \
    while (0)                       \
    X_ENABLE_WARNING(4127)

// Macro to ensure internal representation is as time components
#define REPASTIME(obj)              \
    do {                            \
        if ((obj).Time_ >= 0)       \
            (obj).InternalToTime(); \
    }                               \
    X_DISABLE_WARNING(4127)         \
    while (0)                       \
    X_ENABLE_WARNING(4127)

#define BITOFFSETMILLISECOND 0
#define BITSIZEMILLISECOND 10
#define BITOFFSETSECOND 10
#define BITSIZESECOND 6
#define BITOFFSETMINUTE 16
#define BITSIZEMINUTE 6
#define BITOFFSETHOUR 22
#define BITSIZEHOUR 5

#define MSPERDAY (24 * 60 * 60 * 1000)

#define GETCOMPONENT(v, offset, length) ((v) >> (offset) & ((1 << (length)) - 1))

TimeStamp::TimeStamp(int hour, int minute, int second, int millisecond)
{
    Time_ = millisecond + 1000 * (second + 60 * (minute + 60 * hour));
}

TimeStamp TimeStamp::GetSystemTime(void)
{
    _SYSTEMTIME Time;
    GetLocalTime(&Time);

    return TimeStamp(Time.wHour, Time.wMinute, Time.wSecond, Time.wMilliseconds);
}

int TimeStamp::GetHour(void) const
{
    REPASTIME(*this);
    int Hour = GETCOMPONENT(Time_, BITOFFSETHOUR, BITSIZEHOUR);
    X_ASSERT(Hour >= 0 && Hour < 24, "Hour value is not valid")
    (Hour);
    return Hour;
}

int TimeStamp::GetMinute(void) const
{
    REPASTIME(*this);
    int Minute = GETCOMPONENT(Time_, BITOFFSETMINUTE, BITSIZEMINUTE);
    X_ASSERT(Minute >= 0 && Minute < 60, "Minute value is not valid")
    (Minute);
    return Minute;
}

int TimeStamp::GetSecond(void) const
{
    REPASTIME(*this);
    int Second = GETCOMPONENT(Time_, BITOFFSETSECOND, BITSIZESECOND);
    X_ASSERT(Second >= 0 && Second < 60, "Seconds value is not valid")
    (Second);
    return Second;
}

int TimeStamp::GetMilliSecond(void) const
{
    REPASTIME(*this);
    int MilliSecond = GETCOMPONENT(Time_, BITOFFSETMILLISECOND, BITSIZEMILLISECOND);
    X_ASSERT(MilliSecond >= 0 && MilliSecond < 1000, "Milliseconds value is not valid")
    (MilliSecond);
    return MilliSecond;
}

int TimeStamp::GetMilliSecondsPastMidnight(void) const
{
    REPASMSPM(*this);
    return Time_;
}

const char* TimeStamp::ToString(Description& desc) const
{
    sprintf_s(desc, "%02d:%02d:%02d,%04d", this->GetHour(), this->GetMinute(), this->GetSecond(), this->GetMilliSecond());

    return desc;
}

const char* TimeStamp::ToString(FileDescription& desc) const
{
    sprintf_s(desc, "%02d-%02d-%02d", this->GetHour(), this->GetMinute(), this->GetSecond());

    return desc;
}

void TimeStamp::InternalToMSPM(void) const
{
    X_ASSERT(Time_ >= 0, "Time is negative")
    (Time_);

    int Hour = GETCOMPONENT(Time_, BITOFFSETHOUR, BITSIZEHOUR);
    int Minute = GETCOMPONENT(Time_, BITOFFSETMINUTE, BITSIZEMINUTE);
    int Second = GETCOMPONENT(Time_, BITOFFSETSECOND, BITSIZESECOND);
    int MilliSecond = GETCOMPONENT(Time_, BITOFFSETMILLISECOND, BITSIZEMILLISECOND);
    Time_ = MilliSecond + 1000 * (Second + 60 * (Minute + 60 * Hour));
}

void TimeStamp::InternalToTime(void) const
{
    X_ASSERT(Time_ >= 0, "Time is negative")
    (Time_);

    // Convert to time parts
    int MilliSecond = Time_ % 1000;
    Time_ /= 1000;
    int Second = Time_ % 60;
    Time_ /= 60;
    int Minute = Time_ % 60;
    Time_ /= 60;
    int Hour = Time_;

    // Copy calculated values
    int TimeRep = ((MilliSecond) << BITOFFSETMILLISECOND)
                  + ((Second) << BITOFFSETSECOND)
                  + ((Minute) << BITOFFSETMINUTE)
                  + ((Hour) << BITOFFSETHOUR)
                  + (1 << 31);

    Time_ = TimeRep;
}

X_NAMESPACE_END