#include "EngineCommon.h"


#include "LoggerInternalConsoleFormatPolicy.h"
#include "String\StackString.h"

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
	const char* type, const SourceInfo& sourceInfo, const char* channel,
	size_t verbosity, const char* format, va_list args
	)
{
	int bytesWritten;

	X_UNUSED(indentation);
	X_UNUSED(type);
	X_UNUSED(verbosity);
	X_UNUSED(sourceInfo);

	bytesWritten = _snprintf_s(line, _TRUNCATE, "^4%-12s^7", channel);
	bytesWritten += vsnprintf_s(&line[bytesWritten], sizeof(LoggerBase::Line) - bytesWritten, _TRUNCATE, format, args);
	bytesWritten += _snprintf_s(&line[bytesWritten], sizeof(LoggerBase::Line) - bytesWritten, _TRUNCATE, "\n");

	return safe_static_cast<uint32_t, int>(bytesWritten);
}



X_NAMESPACE_END