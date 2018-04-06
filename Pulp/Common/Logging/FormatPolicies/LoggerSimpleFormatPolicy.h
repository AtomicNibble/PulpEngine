#pragma once

#ifndef X_LOGGERSIMPLEFORMATPOLICY_H_
#define X_LOGGERSIMPLEFORMATPOLICY_H_

#include "Logging\LoggerBase.h"

X_NAMESPACE_BEGIN(core)

/// \ingroup Logging
/// \brief A class that implements a format policy for log messages.
/// \details This class implements the concepts of a format policy as expected by the Logger class. It formats all
/// messages the following way:
/// \code
///   [channel name]    | message
///
///   // an example message
///   [CoreModule]      | Starting module.
/// \endcode
/// The format does not contain any time stamps or source code information, and is mostly useful for logging messages
/// to a constrained destination, such as a console window.
/// \sa X_LOG0 X_LOG1 X_LOG2 X_WARNING X_ERROR X_FATAL Logger LoggerBase
class LoggerSimpleFormatPolicy
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

class LoggerSimpleFormatPolicyStripColors : public LoggerSimpleFormatPolicy
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

#endif // !X_LOGGERSIMPLEFORMATPOLICY_H_
