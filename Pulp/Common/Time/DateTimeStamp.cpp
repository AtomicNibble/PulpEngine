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
    return date > rhs.date || time > rhs.time;
}

bool DateTimeStamp::operator<(const DateTimeStamp& rhs) const
{
    return date < rhs.date || time < rhs.time;
}

const char* DateTimeStamp::toString(Description& desc) const
{
    sprintf_s(desc, "%d-%02d-%02dT%d-%02d-%02d", date.getYear(), date.getMonth(), date.getDay(), 
        time.getHour(), time.getMinute(), time.getSecond());
    return desc;
}

bool DateTimeStamp::fromString(core::string_view str, DateTimeStamp& stampOut)
{
    // make sure it's null term.
    core::StackString<sizeof(Description), char> buf(str.begin(), str.end());

    // will be same format as above.
    int year, month, day, hour, minute, second;

    int num = sscanf_s(buf.c_str(), "%d-%d-%dT%d-%d-%d",
        &year, &month, &day, &hour, &minute, &second);

    if (num != 6) {
        X_ERROR("DateTime", "Failed to parse string \"%s\"", buf.c_str());
        return false;
    }

    stampOut = DateTimeStamp(year, month, day, hour, minute, second, 0);
    return true;
}

DateTimeStamp DateTimeStamp::getSystemDateTime(void)
{
    _SYSTEMTIME Time;
    GetLocalTime(&Time);

    return DateTimeStamp(Time.wYear, Time.wMonth, Time.wDay, Time.wHour, Time.wMinute, Time.wSecond, Time.wMilliseconds);
}

X_NAMESPACE_END

