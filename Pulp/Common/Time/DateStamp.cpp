#include "EngineCommon.h"

#include "DateStamp.h"

#include <stdio.h>

X_NAMESPACE_BEGIN(core)

namespace {

	const static int g_YearDayFromMonth[] = { 0, 1, 32, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 };
	const static int g_DaysPerMonth[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	const static int g_MinYear = 1500;
}


DateStamp::DateStamp(unsigned short year, unsigned char month, unsigned char day) :
	m_year( year ), 
	m_month( month ),
	m_day( day )
{
	// validate it.
	
	X_ASSERT( day >= 1 && day <= DaysInMonth( month ), "Day is not valid for month: %i, max is: ", month, DaysInMonth( month ) )( day );
	X_ASSERT( month >= 1 && month <= 12, "Month is not valid")( month );
	X_ASSERT( year >= g_MinYear && year <= g_MinYear + 1000, "Year is not valid")( year );

}


int DateStamp::GetYear() const
{
	return m_year;
}


int DateStamp::GetQuarter() const
{
	int Quarter = ((m_month-1) / 3) + 1;
	X_ASSERT(Quarter >= 1 && Quarter <= 4, "Quarter is not valid")(Quarter);
	return Quarter;
}


int DateStamp::GetMonth() const
{
	return m_month;
}

int DateStamp::GetDay() const
{
	return m_day;
}

int DateStamp::GetDayOfYear() const
{
	int DayOfYear = g_YearDayFromMonth[ m_month ] + m_day + (IsLeapYear() & (m_month > 2)) - 1;

	X_ASSERT(DayOfYear >= 1 && DayOfYear <= 365 + IsLeapYear(), "Not a valid Day" )(DayOfYear);
	return DayOfYear;
}

int DateStamp::IsLeapYear() const
{
	return (m_year % 4 == 0) & ((m_year % 100 != 0) | (m_year % 400 == 0));
}


int DateStamp::DaysInMonth( int month ) const
{
	return (g_DaysPerMonth[month] + ((month)==2 && (IsLeapYear())));
}


DateStamp DateStamp::GetSystemDate(void)
{
	_SYSTEMTIME Time;
	GetLocalTime( &Time );

	return DateStamp( Time.wYear, safe_static_cast<BYTE,WORD>( Time.wMonth ), safe_static_cast<BYTE,WORD>( Time.wDay ) );
}

const char* DateStamp::ToString(Description& desc) const
{
	sprintf_s( desc, "%d-%02d-%02d", m_year, m_month, m_day );
	return desc;
}



X_NAMESPACE_END