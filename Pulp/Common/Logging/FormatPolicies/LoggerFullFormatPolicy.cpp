#include "EngineCommon.h"

#include "Logging\LoggerBase.h"
#include "LoggerFullFormatPolicy.h"
#include <stdio.h>

#include "Time\DateStamp.h"
#include "Time\TimeStamp.h"

X_NAMESPACE_BEGIN(core)


/// Empty implementation.
void LoggerFullFormatPolicy::Init(void)
{

}

/// Empty implementation.
void LoggerFullFormatPolicy::Exit(void)
{

}

/// Formats the given message.
uint32_t LoggerFullFormatPolicy::Format(LoggerBase::Line& line, const char* indentation, 
	const char* type, const SourceInfo& sourceInfo, const char* channel, 
	size_t verbosity, const char* format, va_list args)
{
	int bytesWritten; 


	DateStamp::Description DateStr;
	DateStamp date = DateStamp::GetSystemDate();
	date.ToString(DateStr);

	TimeStamp::Description TimeStr;
	TimeStamp time = TimeStamp::GetSystemTime();
	time.ToString(TimeStr);


	bytesWritten = _snprintf_s( line, _TRUNCATE, "[%s %s] %s(%d): [%s:%s] %s", 
		DateStr, TimeStr,
		sourceInfo.file_, sourceInfo.line_, sourceInfo.module_, channel,
	//	type, verbosity, 
		indentation );

	bytesWritten += vsnprintf_s(&line[bytesWritten], sizeof(LoggerBase::Line) - bytesWritten, _TRUNCATE, format, args);
	bytesWritten += _snprintf_s(&line[bytesWritten], sizeof(LoggerBase::Line) - bytesWritten, _TRUNCATE, "\n");

	return safe_static_cast<int,uint32_t>( bytesWritten );

	return 0;
}



X_NAMESPACE_END