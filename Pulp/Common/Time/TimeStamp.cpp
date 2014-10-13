#include "EngineCommon.h"

#include "TimeStamp.h"

#include <stdio.h>

X_NAMESPACE_BEGIN(core)

// Macro to ensure internal representation is as mill-seconds past midnight
#define REPASMSPM(obj) \
do { \
	if ((obj).m_Time <  0) \
		(obj).InternalToMSPM(); \
} while (0)

// Macro to ensure internal representation is as time components
#define REPASTIME(obj) \
do { \
	if ((obj).m_Time >= 0) \
		(obj).InternalToTime(); \
} while (0)


#define BITOFFSETMILLISECOND 0
#define BITSIZEMILLISECOND 10
#define BITOFFSETSECOND 10
#define BITSIZESECOND 6
#define BITOFFSETMINUTE 16
#define BITSIZEMINUTE 6
#define BITOFFSETHOUR 22
#define BITSIZEHOUR 5

#define MSPERDAY (24*60*60*1000)

#define GETCOMPONENT(v, offset, length) ( (v)>>(offset) & ((1<<(length))-1) )



TimeStamp::TimeStamp( int hour, int minute, int second, int millisecond )
{
	m_Time = millisecond + 1000*(second + 60*(minute + 60*hour));
}


TimeStamp TimeStamp::GetSystemTime()
{
	_SYSTEMTIME Time;
	GetLocalTime( &Time );

	return TimeStamp( Time.wHour, Time.wMinute, Time.wSecond, Time.wMilliseconds );
}


int TimeStamp::GetHour() const
{
	REPASTIME(*this);
	int Hour = GETCOMPONENT(m_Time, BITOFFSETHOUR, BITSIZEHOUR);
	X_ASSERT(Hour >= 0 && Hour < 24, "Hour value is not valid")(Hour);
	return Hour;
}


int TimeStamp::GetMinute() const
{
	REPASTIME(*this);
	int Minute = GETCOMPONENT(m_Time, BITOFFSETMINUTE, BITSIZEMINUTE);
	X_ASSERT(Minute >= 0 && Minute < 60, "Minute value is not valid")(Minute);
	return Minute;
}


int TimeStamp::GetSecond() const
{
	REPASTIME(*this);
	int Second = GETCOMPONENT(m_Time, BITOFFSETSECOND, BITSIZESECOND);
	X_ASSERT(Second >= 0 && Second < 60, "Seconds value is not valid")(Second);
	return Second;
}


int TimeStamp::GetMilliSecond() const
{
	REPASTIME(*this);
	int MilliSecond = GETCOMPONENT(m_Time, BITOFFSETMILLISECOND, BITSIZEMILLISECOND);
	X_ASSERT(MilliSecond >= 0 && MilliSecond < 1000, "Milliseconds value is not valid" )(MilliSecond);
	return MilliSecond;
}

int TimeStamp::GetMilliSecondsPastMidnight() const
{
	REPASMSPM(*this);
	return m_Time;
}


const char* TimeStamp::ToString(Description& desc) const
{
	sprintf_s( desc, "%02d:%02d:%02d,%04d", this->GetHour(), this->GetMinute(), this->GetSecond(), this->GetMilliSecond() );

	return desc;
}

const char* TimeStamp::ToString(FileDescription& desc) const
{
	sprintf_s( desc, "%02d-%02d-%02d", this->GetHour(), this->GetMinute(), this->GetSecond() );

	return desc;	
}
	


void TimeStamp::InternalToMSPM() const
{
	X_ASSERT(m_Time >= 0, "Time is negative")(m_Time);

	int Hour = GETCOMPONENT(m_Time, BITOFFSETHOUR, BITSIZEHOUR);
	int Minute = GETCOMPONENT(m_Time, BITOFFSETMINUTE, BITSIZEMINUTE);
	int Second = GETCOMPONENT(m_Time, BITOFFSETSECOND, BITSIZESECOND);
	int MilliSecond = GETCOMPONENT(m_Time, BITOFFSETMILLISECOND, BITSIZEMILLISECOND);
	m_Time = MilliSecond + 1000*(Second + 60*(Minute + 60*Hour));
}


void TimeStamp::InternalToTime() const
{
	X_ASSERT(m_Time >= 0, "Time is negative")(m_Time);

	// Convert to time parts
	int MilliSecond = m_Time % 1000; m_Time /= 1000;
	int Second = m_Time % 60; m_Time /= 60;
	int Minute = m_Time % 60; m_Time /= 60;
	int Hour = m_Time;

	// Copy calculated values
	int TimeRep =
		  ((MilliSecond) << BITOFFSETMILLISECOND)
		+ ((Second) << BITOFFSETSECOND)
		+ ((Minute) << BITOFFSETMINUTE)
		+ ((Hour) << BITOFFSETHOUR)
		+ (1 << 31);

	m_Time = TimeRep;
}


X_NAMESPACE_END