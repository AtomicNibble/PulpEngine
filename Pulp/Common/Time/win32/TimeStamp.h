#pragma once

#ifndef X_TIMESTAMP_H_
#define X_TIMESTAMP_H_

X_NAMESPACE_BEGIN(core)

class TimeStamp
{
public:
    typedef char Description[14];
    typedef char FileDescription[9];

public:
    TimeStamp();
    TimeStamp(int hour, int minute, int second, int millisecond = 0);

public:

    bool operator>(const TimeStamp& rhs) const;
    bool operator<(const TimeStamp& rhs) const;

    int getHour(void) const;
    int getMinute(void) const;
    int getSecond(void) const;
    int getMilliSecond(void) const;

    int getMilliSecondsPastMidnight(void) const;

    const char* toString(Description& desc) const;
    const char* toString(FileDescription& desc) const;

    static TimeStamp getSystemTime(void);

private:
    int32_t internalToMSPM(void) const;
    int32_t toTime(int hour, int minute, int second, int millisecond) const;

    int32_t time_;
};

X_NAMESPACE_END

#endif // X_TIMESTAMP_H_
