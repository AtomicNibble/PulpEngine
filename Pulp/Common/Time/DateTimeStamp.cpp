#include "EngineCommon.h"
#include "DateTimeStamp.h"

X_NAMESPACE_BEGIN(core)


DateTimeStamp::DateTimeStamp(int year, int month, int day, int hour, int minute, int second, int millisecond) :
    date(year, month, day),
    time(hour, minute, second, millisecond)
{

}


DateTimeStamp::DateTimeStamp(const DateStamp& date_, const TimeStamp& time_) :
    date(date_),
    time(time_)
{
    
}

bool DateTimeStamp::operator>(const DateTimeStamp& rhs) const
{
    return date > rhs.date && time > rhs.time;
}

bool DateTimeStamp::operator<(const DateTimeStamp& rhs) const
{
    return date < rhs.date && time < rhs.time;
}

const char* DateTimeStamp::toString(Description& desc) const
{
    sprintf_s(desc, "%d-%02d-%02dT%d-%02d-%02d", date.getYear(), date.getMonth(), date.getDay(), 
        time.getHour(), time.getMinute(), time.getSecond());
    return desc;
}

DateTimeStamp DateTimeStamp::getSystemDateTime(void)
{
    return DateTimeStamp(DateStamp::getSystemDate(), TimeStamp::getSystemTime());
}

X_NAMESPACE_END

