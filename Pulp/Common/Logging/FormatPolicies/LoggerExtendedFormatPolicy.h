#pragma once

#ifndef X_LOGGEREXTENDEDFORMATPOLICY_H_
#define X_LOGGEREXTENDEDFORMATPOLICY_H_

X_NAMESPACE_BEGIN(core)

/// \ingroup Logging
/// \brief A class that implements a format policy for log messages.
/// \details This class implements the concepts of a format policy as expected by the Logger class. It formats all
/// messages the following way:
/// \code
///   source file(source line): [channel name] (message type) | message
///
///   // an example message
///   ..\..\src\Core\CoreModule.cpp(20): [CoreModule] (INFO0) | Starting module.
/// \endcode
/// The format is extremely useful when used in combination with a LoggerDebuggerWritePolicy because the messages are
/// formatted in such a way that double-clicking on any log message will open the corresponding source file at the
/// original location in the IDE.
/// \sa X_LOG0 X_LOG1 X_LOG2 X_WARNING X_ERROR X_FATAL Logger LoggerBase
class LoggerExtendedFormatPolicy
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

#endif // X_LOGGEREXTENDEDFORMATPOLICY_H_
