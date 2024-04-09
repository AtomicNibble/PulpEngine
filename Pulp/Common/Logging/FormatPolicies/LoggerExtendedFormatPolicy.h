#pragma once

#ifndef X_LOGGEREXTENDEDFORMATPOLICY_H_
#define X_LOGGEREXTENDEDFORMATPOLICY_H_

X_NAMESPACE_BEGIN(core)

class LoggerExtendedFormatPolicy
{
public:
    void Init(void);
    void Exit(void);

    // Formats the given message.
    uint32_t Format(LoggerBase::Line& line, const char* indentation, LogType::Enum type,
        X_SOURCE_INFO_LOG_CA(const SourceInfo& sourceInfo) const char* channel, size_t verbosity,
        const char* format, va_list args);
};

X_NAMESPACE_END

#endif // X_LOGGEREXTENDEDFORMATPOLICY_H_
