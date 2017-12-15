#include "EngineCommon.h"


#include "LoggerInternalConsoleFormatPolicy.h"

#include <stdio.h>

X_NAMESPACE_BEGIN(core)


/// Empty implementation.
void LoggerInternalConsoleFormatPolicy::Init(void)
{

}

/// Empty implementation.
void LoggerInternalConsoleFormatPolicy::Exit(void)
{

}

/// Formats the given message.
uint32_t LoggerInternalConsoleFormatPolicy::Format(LoggerBase::Line& line, const char* indentation,
	LogType::Enum type, X_SOURCE_INFO_LOG_CA(const SourceInfo&) const char* channel,
	size_t verbosity, const char* format, va_list args)
{
	X_UNUSED(indentation, type, verbosity);

	int bytesWritten;
	char colorCode = '7';

	if (type != LogType::INFO) 
	{
		if (type == LogType::WARNING) {
			colorCode = '6';
		}
		else  {
			colorCode = '1';
		}
	} 

	bytesWritten = _snprintf_s(line, _TRUNCATE, "^4%-15s^%c%s", channel, colorCode, indentation);
	bytesWritten += vsnprintf_s(&line[bytesWritten], sizeof(LoggerBase::Line) - bytesWritten, _TRUNCATE, format, args);
	bytesWritten += _snprintf_s(&line[bytesWritten], sizeof(LoggerBase::Line) - bytesWritten, _TRUNCATE, "\n");

	return safe_static_cast<uint32_t, int>(bytesWritten);
}



X_NAMESPACE_END