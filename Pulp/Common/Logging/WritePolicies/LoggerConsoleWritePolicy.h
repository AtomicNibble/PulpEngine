#pragma once

#ifndef X_LOGGERCONSOLEWRITEPOLICY_H_
#define X_LOGGERCONSOLEWRITEPOLICY_H_

#include "Threading/CriticalSection.h"

#include "Logging\LoggerBase.h"

X_NAMESPACE_BEGIN(core)

class Console;

/// \ingroup Logging
/// \brief A class that implements a write policy for log messages.
/// \details This class implements the concepts of a write policy as expected by the Logger class. It writes all
/// log messages to a console window.
/// \sa X_LOG0 X_LOG1 X_LOG2 X_WARNING X_ERROR X_FATAL Logger LoggerBase
class LoggerConsoleWritePolicy
{
public:
    /// \brief Constructs a policy instance.
    /// \remark The policy stores a pointer to the console internally, but does not take ownership. Users must ensure
    /// that the console is not freed before the policy is.
    explicit LoggerConsoleWritePolicy(const Console& console);

    /// Initializes a custom color table for the console.
    void Init(void);

    /// Empty implementation.
    void Exit(void);

    /// Writes a log message to the console.
    void WriteLog(const LoggerBase::Line& line, uint32_t length);

    /// Writes a warning message to the console.
    void WriteWarning(const LoggerBase::Line& line, uint32_t length);

    /// Writes an error message to the console.
    void WriteError(const LoggerBase::Line& line, uint32_t length);

    /// Writes a fatal error message to the console.
    void WriteFatal(const LoggerBase::Line& line, uint32_t length);

    /// Writes an assert message to the console.
    void WriteAssert(const LoggerBase::Line& line, uint32_t length);

    /// Writes an assert variable message to the console.
    void WriteAssertVariable(const LoggerBase::Line& line, uint32_t length);

private:
    HANDLE console_;
    CriticalSection cs_;
};

X_NAMESPACE_END

#endif // X_LOGGERCONSOLEWRITEPOLICY_H_
