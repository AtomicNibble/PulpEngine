#include "EngineCommon.h"

#include "DateStamp.h"

#include <stdio.h>

X_NAMESPACE_BEGIN(core)

namespace
{
    const static int g_YearDayFromMonth[] = {0, 1, 32, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366};
    const static int g_DaysPerMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    const static int g_MinYear = 1500;
} // namespace

DateStamp::DateStamp(unsigned short year, unsigned char month, unsigned char day) :
    year_(year),
    month_(month),
    day_(day)
{
    // validate it.

    X_ASSERT(day >= 1 && day <= DaysInMonth(month), "Day is not valid for month: %i, max is: ", month, DaysInMonth(month))
    (day);
    X_ASSERT(month >= 1 && month <= 12, "Month is not valid")
    (month);
    X_ASSERT(year >= g_MinYear && year <= g_MinYear + 1000, "Year is not valid")
    (year);
}

int DateStamp::GetYear() const
{
    return year_;
}

int DateStamp::GetQuarter() const
{
    int Quarter = ((month_ - 1) / 3) + 1;
    X_ASSERT(Quarter >= 1 && Quarter <= 4, "Quarter is not valid")
    (Quarter);
    return Quarter;
}

int DateStamp::GetMonth() const
{
    return month_;
}

int DateStamp::GetDay() const
{
    return day_;
}

int DateStamp::GetDayOfYear() const
{
    int DayOfYear = g_YearDayFromMonth[month_] + day_ + (IsLeapYear() & (month_ > 2)) - 1;

    X_ASSERT(DayOfYear >= 1 && DayOfYear <= 365 + IsLeapYear(), "Not a valid Day")
    (DayOfYear);
    return DayOfYear;
}

int DateStamp::IsLeapYear() const
{
    return (year_ % 4 == 0) & ((year_ % 100 != 0) | (year_ % 400 == 0));
}

int DateStamp::DaysInMonth(int month) const
{
    return (g_DaysPerMonth[month] + ((month) == 2 && (IsLeapYear())));
}

DateStamp DateStamp::GetSystemDate(void)
{
    _SYSTEMTIME Time;
    GetLocalTime(&Time);

    return DateStamp(Time.wYear, safe_static_cast<BYTE, WORD>(Time.wMonth), safe_static_cast<BYTE, WORD>(Time.wDay));
}

const char* DateStamp::ToString(Description& desc) const
{
    sprintf_s(desc, "%d-%02d-%02d", year_, month_, day_);
    return desc;
}

X_NAMESPACE_END