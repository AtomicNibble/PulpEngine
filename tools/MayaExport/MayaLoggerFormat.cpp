#include "stdafx.h"
#include "MayaLoggerFormat.h"


X_NAMESPACE_BEGIN(maya)


/// Empty implementation.
void LoggerMayaFormatPolicy::Init(void)
{

}

/// Empty implementation.
void LoggerMayaFormatPolicy::Exit(void)
{

}

/// Formats the given message.
uint32_t LoggerMayaFormatPolicy::Format(core::LoggerBase::Line& line, const char* indentation,
	const char* type, const core::SourceInfo& sourceInfo, const char* channel,
	size_t verbosity, const char* format, va_list args
	)
{
	int bytesWritten = 0;

	X_UNUSED(type);
	X_UNUSED(sourceInfo);
	X_UNUSED(verbosity);
	X_UNUSED(channel);
	X_UNUSED(indentation);

//	bytesWritten = _snprintf_s(line, _TRUNCATE, "%-20s%s", channel, indentation);
	bytesWritten += vsnprintf_s(&line[bytesWritten], sizeof(core::LoggerBase::Line) - bytesWritten, _TRUNCATE, format, args);

	return safe_static_cast<uint32_t, int>(bytesWritten);
}



X_NAMESPACE_END