#pragma once

#include "DateStamp.h"
#include "TimeStamp.h"

X_NAMESPACE_BEGIN(core)

struct DateTimeStamp
{
    typedef char Description[64];

public:
    DateTimeStamp() = default;
    DateTimeStamp(int year, int month, int day, int hour, int minute, int second, int millisecond);
    DateTimeStamp(const DateStamp& date, const TimeStamp& time);

    bool operator>(const DateTimeStamp& rhs) const;
    bool operator<(const DateTimeStamp& rhs) const;

    const char* toString(Description& desc) const;

    static DateTimeStamp getSystemDateTime(void);

public:
    core::DateStamp date;
    core::TimeStamp time;
};

X_NAMESPACE_END

