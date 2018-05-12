#include "EngineCommon.h"

#include "DateStamp.h"

#include <stdio.h>

X_NAMESPACE_BEGIN(core)

namespace
{
    const int g_YearDayFromMonth[] = {0, 1, 32, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366};
    const int g_DaysPerMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    const int g_MinYear = 1500;
} // namespace

DateStamp::DateStamp()
{
    year_ = 0;
    month_ = 0;
    day_ = 0;
}


DateStamp::DateStamp(int year, int month, int day) :
    year_(safe_static_cast<decltype(year_)>(year)),
    month_(safe_static_cast<decltype(month_)>(month)),
    day_(safe_static_cast<decltype(day_)>(day))
{
    // validate it.

    X_ASSERT(day >= 1 && day <= daysInMonth(month), "Day is not valid for month: %i, max is: ", month, daysInMonth(month))(day);
    X_ASSERT(month >= 1 && month <= 12, "Month is not valid")(month);
    X_ASSERT(year >= g_MinYear && year <= g_MinYear + 1000, "Year is not valid")(year);
}

bool DateStamp::operator>(const DateStamp& rhs) const
{
    return year_ > rhs.year_ || month_ > rhs.year_ || day_ > rhs.day_;
}

bool DateStamp::operator<(const DateStamp& rhs) const
{
    return year_ < rhs.year_ || month_ < rhs.year_ || day_ < rhs.day_;
}


int DateStamp::getYear(void) const
{
    return year_;
}

int DateStamp::getQuarter(void) const
{
    int Quarter = ((month_ - 1) / 3) + 1;
    X_ASSERT(Quarter >= 1 && Quarter <= 4, "Quarter is not valid")(Quarter);
    return Quarter;
}

int DateStamp::getMonth(void) const
{
    return month_;
}

int DateStamp::getDay(void) const
{
    return day_;
}

int DateStamp::getDayOfYear(void) const
{
    int DayOfYear = g_YearDayFromMonth[month_] + day_ + (isLeapYear() & (month_ > 2)) - 1;

    X_ASSERT(DayOfYear >= 1 && DayOfYear <= 365 + isLeapYear(), "Not a valid Day")(DayOfYear);
    return DayOfYear;
}

int DateStamp::isLeapYear(void) const
{
    return (year_ % 4 == 0) & ((year_ % 100 != 0) | (year_ % 400 == 0));
}

int DateStamp::daysInMonth(int month) const
{
    return (g_DaysPerMonth[month] + ((month) == 2 && (isLeapYear())));
}

DateStamp DateStamp::getSystemDate(void)
{
    _SYSTEMTIME Time;
    GetLocalTime(&Time);

    return DateStamp(Time.wYear, Time.wMonth, Time.wDay);
}

const char* DateStamp::toString(Description& desc) const
{
    sprintf_s(desc, "%d-%02d-%02d", year_, month_, day_);
    return desc;
}

X_NAMESPACE_END