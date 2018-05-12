#include "EngineCommon.h"

#include "TimeStamp.h"

#include <stdio.h>

X_NAMESPACE_BEGIN(core)

#define BITOFFSETMILLISECOND 0
#define BITSIZEMILLISECOND 10
#define BITOFFSETSECOND 10
#define BITSIZESECOND 6
#define BITOFFSETMINUTE 16
#define BITSIZEMINUTE 6
#define BITOFFSETHOUR 22
#define BITSIZEHOUR 5

#define GETCOMPONENT(v, offset, length) ((v) >> (offset) & ((1 << (length)) - 1))

TimeStamp::TimeStamp()
{
    time_ = 0;
}

TimeStamp::TimeStamp(int hour, int minute, int second, int millisecond)
{
    time_ = toTime(hour, minute, second, millisecond);

    X_ASSERT(hour == getHour(), "Decode error")(hour, getHour());
    X_ASSERT(minute == getMinute(), "Decode error")(minute, getMinute());
    X_ASSERT(second == getSecond(), "Decode error")(second, getSecond());
    X_ASSERT(millisecond == getMilliSecond(), "Decode error")(millisecond, getMilliSecond());

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
    int hour = GETCOMPONENT(time_, BITOFFSETHOUR, BITSIZEHOUR);
    X_ASSERT(hour >= 0 && hour < 24, "Hour value is not valid")(hour);
    return hour;
}

int TimeStamp::getMinute(void) const
{
    int minute = GETCOMPONENT(time_, BITOFFSETMINUTE, BITSIZEMINUTE);
    X_ASSERT(minute >= 0 && minute < 60, "Minute value is not valid")(minute);
    return minute;
}

int TimeStamp::getSecond(void) const
{
    int second = GETCOMPONENT(time_, BITOFFSETSECOND, BITSIZESECOND);
    X_ASSERT(second >= 0 && second < 60, "Seconds value is not valid")(second);
    return second;
}

int TimeStamp::getMilliSecond(void) const
{
    int millisecond = GETCOMPONENT(time_, BITOFFSETMILLISECOND, BITSIZEMILLISECOND);
    X_ASSERT(millisecond >= 0 && millisecond < 1000, "Milliseconds value is not valid")(millisecond);
    return millisecond;
}

int TimeStamp::getMilliSecondsPastMidnight(void) const
{
    return internalToMSPM();
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

int32_t TimeStamp::internalToMSPM(void) const
{
    X_ASSERT(time_ >= 0, "Time is negative")(time_);

    int Hour = GETCOMPONENT(time_, BITOFFSETHOUR, BITSIZEHOUR);
    int Minute = GETCOMPONENT(time_, BITOFFSETMINUTE, BITSIZEMINUTE);
    int Second = GETCOMPONENT(time_, BITOFFSETSECOND, BITSIZESECOND);
    int MilliSecond = GETCOMPONENT(time_, BITOFFSETMILLISECOND, BITSIZEMILLISECOND);
    return MilliSecond + 1000 * (Second + 60 * (Minute + 60 * Hour));
}

int32_t TimeStamp::toTime(int hour, int minute, int second, int millisecond) const
{
    int millisecondShifed = (millisecond << BITOFFSETMILLISECOND);
    int secondShifed = (second << BITOFFSETSECOND);
    int minuteShifed = (minute << BITOFFSETMINUTE);
    int housrShifed = (hour << BITOFFSETHOUR);

    return millisecondShifed | secondShifed | minuteShifed | housrShifed;
}

X_NAMESPACE_END