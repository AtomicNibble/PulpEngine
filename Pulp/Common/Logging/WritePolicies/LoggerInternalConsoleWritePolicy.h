#pragma once

#ifndef X_LOGGER_INTERNAL_CONSOLE_WRITE_POLICY_H_
#define X_LOGGER_INTERNAL_CONSOLE_WRITE_POLICY_H_

#include "Logging\LoggerBase.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(core)

class LoggerInternalConsoleWritePolicy
{
public:
    explicit LoggerInternalConsoleWritePolicy();

    /// Empty implementation.
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
    ICore* pCore_;
};

X_NAMESPACE_END

#endif // !X_LOGGER_INTERNAL_CONSOLE_WRITE_POLICY_H_
