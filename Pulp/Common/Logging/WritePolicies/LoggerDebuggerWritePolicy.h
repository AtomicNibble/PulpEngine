#pragma once

#ifndef X_LOGGERDEBUGGERWRITEPOLICY_H_
#define X_LOGGERDEBUGGERWRITEPOLICY_H_

#include "Logging\LoggerBase.h"

X_NAMESPACE_BEGIN(core)

/// \ingroup Logging
/// \brief A class that implements a write policy for log messages.
/// \details This class implements the concepts of a write policy as expected by the Logger class. It writes all
/// log messages to the debugger output/TTY window.
/// \sa X_LOG0 X_LOG1 X_LOG2 X_WARNING X_ERROR X_FATAL Logger LoggerBase
class LoggerDebuggerWritePolicy
{
public:
    /// Empty implementation.
    void Init(void);

    /// Empty implementation.
    void Exit(void);

    /// Writes a log message to the debugger.
    void WriteLog(const LoggerBase::Line& line, uint32_t length);

    /// Writes a warning message to the debugger.
    void WriteWarning(const LoggerBase::Line& line, uint32_t length);

    /// Writes an error message to the debugger.
    void WriteError(const LoggerBase::Line& line, uint32_t length);

    /// Writes a fatal error message to the debugger.
    void WriteFatal(const LoggerBase::Line& line, uint32_t length);

    /// Writes an assert message to the debugger.
    void WriteAssert(const LoggerBase::Line& line, uint32_t length);

    /// Writes an assert variable message to the debugger.
    void WriteAssertVariable(const LoggerBase::Line& line, uint32_t length);
};

X_NAMESPACE_END

#endif // X_LOGGERDEBUGGERWRITEPOLICY_H_
