#include <EngineCommon.h>
#include "CompressedStamps.h"


X_NAMESPACE_BEGIN(core)

dateStampSmall dateStampSmall::systemDate(void)
{
	_SYSTEMTIME time;
	GetLocalTime(&time);

	return dateStampSmall(time.wYear,time.wMonth,time.wDay);
}


TimeStampSmall TimeStampSmall::systemTime(void)
{
	_SYSTEMTIME time;
	GetLocalTime(&time);

	return TimeStampSmall(time.wHour, time.wMinute, time.wSecond);
}


dateTimeStampSmall dateTimeStampSmall::systemDateTime(void)
{
	_SYSTEMTIME time;
	GetLocalTime(&time);

	return dateTimeStampSmall(time.wYear, time.wMonth, time.wDay,
		time.wHour, time.wMinute, time.wSecond);
}


X_NAMESPACE_END