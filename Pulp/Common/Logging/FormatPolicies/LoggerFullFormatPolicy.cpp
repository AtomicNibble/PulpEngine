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
	LogType::Enum type, X_SOURCE_INFO_LOG_CA(const SourceInfo& sourceInfo)
	const char* channel, size_t verbosity, const char* format, va_list args)
{
	X_UNUSED(type, verbosity);
	

	DateStamp::Description DateStr;
	DateStamp date = DateStamp::GetSystemDate();
	date.ToString(DateStr);

	TimeStamp::Description TimeStr;
	TimeStamp time = TimeStamp::GetSystemTime();
	time.ToString(TimeStr);

#if X_ENABLE_LOGGING_SOURCE_INFO
	int32_t bytesWritten = _snprintf_s( line, _TRUNCATE, "[%s %s] %s(%d): [%s:%s] %s", 
		DateStr, TimeStr,
		sourceInfo.file_, sourceInfo.line_, sourceInfo.module_, 
		channel, indentation
	);
#else
	int32_t bytesWritten = _snprintf_s(line, _TRUNCATE, "[%s %s] : [%s] %s", DateStr, TimeStr, channel, indentation);
#endif // !X_ENABLE_LOGGING_SOURCE_INFO

	bytesWritten += vsnprintf_s(&line[bytesWritten], sizeof(LoggerBase::Line) - bytesWritten, _TRUNCATE, format, args);
	bytesWritten += _snprintf_s(&line[bytesWritten], sizeof(LoggerBase::Line) - bytesWritten, _TRUNCATE, "\n");

	return safe_static_cast<uint32_t, int>(bytesWritten);
}



X_NAMESPACE_END