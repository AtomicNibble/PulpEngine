#pragma once

#ifndef X_LOGGER_INTERNAL_CONSOLE_FORMAT_POLICY_H_
#define X_LOGGER_INTERNAL_CONSOLE_FORMAT_POLICY_H_

#include "Logging\LoggerBase.h"

X_NAMESPACE_BEGIN(core)

class LoggerInternalConsoleFormatPolicy
{
public:
    /// Empty implementation.
    void Init(void);

    /// Empty implementation.
    void Exit(void);

    /// Formats the given message.
    uint32_t Format(LoggerBase::Line& line, const char* indentation, LogType::Enum type,
        X_SOURCE_INFO_LOG_CA(const SourceInfo& sourceInfo) const char* channel, size_t verbosity,
        const char* format, va_list args);
};

X_NAMESPACE_END

#endif // !X_LOGGER_INTERNAL_CONSOLE_FORMAT_POLICY_H_
