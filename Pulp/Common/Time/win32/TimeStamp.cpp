#include "EngineCommon.h"

#include "TimeStamp.h"

#include <stdio.h>

X_NAMESPACE_BEGIN(core)

// Macro to ensure internal representation is as mill-seconds past midnight
#define REPASMSPM(obj)              \
    do {                            \
        if ((obj).time_ < 0)        \
            (obj).internalToMSPM(); \
    }                               \
    X_DISABLE_WARNING(4127)         \
    while (0)                       \
    X_ENABLE_WARNING(4127)

// Macro to ensure internal representation is as time components
#define REPASTIME(obj)              \
    do {                            \
        if ((obj).time_ >= 0)       \
            (obj).internalToTime(); \
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

TimeStamp::TimeStamp()
{
    time_ = 0;
}

TimeStamp::TimeStamp(int hour, int minute, int second, int millisecond)
{
    time_ = millisecond + 1000 * (second + 60 * (minute + 60 * hour));
}

bool TimeStamp::operator>(const TimeStamp& rhs) const
{
    return time_ > rhs.time_;
}

bool TimeStamp::operator<(const TimeStamp& rhs) const
{
    return time_ < rhs.time_;
}


TimeStamp TimeStamp::getSystemTime(void)
{
    _SYSTEMTIME Time;
    GetLocalTime(&Time);

    return TimeStamp(Time.wHour, Time.wMinute, Time.wSecond, Time.wMilliseconds);
}

int TimeStamp::getHour(void) const
{
    REPASTIME(*this);
    int Hour = GETCOMPONENT(time_, BITOFFSETHOUR, BITSIZEHOUR);
    X_ASSERT(Hour >= 0 && Hour < 24, "Hour value is not valid")(Hour);
    return Hour;
}

int TimeStamp::getMinute(void) const
{
    REPASTIME(*this);
    int Minute = GETCOMPONENT(time_, BITOFFSETMINUTE, BITSIZEMINUTE);
    X_ASSERT(Minute >= 0 && Minute < 60, "Minute value is not valid")(Minute);
    return Minute;
}

int TimeStamp::getSecond(void) const
{
    REPASTIME(*this);
    int Second = GETCOMPONENT(time_, BITOFFSETSECOND, BITSIZESECOND);
    X_ASSERT(Second >= 0 && Second < 60, "Seconds value is not valid")(Second);
    return Second;
}

int TimeStamp::getMilliSecond(void) const
{
    REPASTIME(*this);
    int MilliSecond = GETCOMPONENT(time_, BITOFFSETMILLISECOND, BITSIZEMILLISECOND);
    X_ASSERT(MilliSecond >= 0 && MilliSecond < 1000, "Milliseconds value is not valid")(MilliSecond);
    return MilliSecond;
}

int TimeStamp::getMilliSecondsPastMidnight(void) const
{
    REPASMSPM(*this);
    return time_;
}

const char* TimeStamp::toString(Description& desc) const
{
    sprintf_s(desc, "%02d:%02d:%02d,%04d", getHour(), getMinute(), getSecond(), getMilliSecond());

    return desc;
}

const char* TimeStamp::toString(FileDescription& desc) const
{
    sprintf_s(desc, "%02d-%02d-%02d", getHour(), getMinute(), getSecond());

    return desc;
}

void TimeStamp::internalToMSPM(void) const
{
    X_ASSERT(time_ >= 0, "Time is negative")(time_);

    int Hour = GETCOMPONENT(time_, BITOFFSETHOUR, BITSIZEHOUR);
    int Minute = GETCOMPONENT(time_, BITOFFSETMINUTE, BITSIZEMINUTE);
    int Second = GETCOMPONENT(time_, BITOFFSETSECOND, BITSIZESECOND);
    int MilliSecond = GETCOMPONENT(time_, BITOFFSETMILLISECOND, BITSIZEMILLISECOND);
    time_ = MilliSecond + 1000 * (Second + 60 * (Minute + 60 * Hour));
}

void TimeStamp::internalToTime(void) const
{
    X_ASSERT(time_ >= 0, "Time is negative")(time_);

    // Convert to time parts
    int MilliSecond = time_ % 1000;
    time_ /= 1000;
    int Second = time_ % 60;
    time_ /= 60;
    int Minute = time_ % 60;
    time_ /= 60;
    int Hour = time_;

    // Copy calculated values
    int TimeRep = ((MilliSecond) << BITOFFSETMILLISECOND)
                  + ((Second) << BITOFFSETSECOND)
                  + ((Minute) << BITOFFSETMINUTE)
                  + ((Hour) << BITOFFSETHOUR)
                  + (1 << 31);

    time_ = TimeRep;
}

X_NAMESPACE_END