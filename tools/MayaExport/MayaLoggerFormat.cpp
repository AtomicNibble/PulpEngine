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
    core::LogType::Enum type, X_SOURCE_INFO_LOG_CA(const SourceInfo&) const char* channel, size_t verbosity, const char* format, va_list args)
{
    X_UNUSED(type, verbosity, channel, indentation);

    int bytesWritten = 0;

    bytesWritten += vsnprintf_s(&line[bytesWritten], sizeof(core::LoggerBase::Line) - bytesWritten, _TRUNCATE, format, args);

    return safe_static_cast<uint32_t>(bytesWritten);
}

X_NAMESPACE_END