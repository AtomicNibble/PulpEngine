#pragma once

#ifndef X_LOGGERFULLFORMATPOLICY_H_
#define X_LOGGERFULLFORMATPOLICY_H_

X_NAMESPACE_BEGIN(core)

/// \ingroup Logging
/// \brief A class that implements a format policy for log messages.
/// \details This class implements the concepts of a format policy as expected by the Logger class. It formats all
/// messages the following way:
/// \code
///   [date time] source file(source line): [channel name] (message type, verbosity) message
///
///   // an example message
///   [2012-01-01 01:01:01,0000] ..\..\src\Core\CoreModule.cpp(20): [CoreModule] (INFO, 0) Starting module.
/// \endcode
/// This format includes date and time stamps as well as all of the provided source code information, and thus is useful
/// for unconstrained output destinations like files. Having date and time stamps can be especially useful in post-mortem
/// debugging.
/// \sa X_LOG0 X_LOG1 X_LOG2 X_WARNING X_ERROR X_FATAL Logger LoggerBase
class LoggerFullFormatPolicy
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

#endif // X_LOGGERFULLFORMATPOLICY_H_
