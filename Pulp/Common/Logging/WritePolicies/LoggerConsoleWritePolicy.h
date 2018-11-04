#pragma once

#ifndef X_LOGGERCONSOLEWRITEPOLICY_H_
#define X_LOGGERCONSOLEWRITEPOLICY_H_

#include "Threading/CriticalSection.h"

#include "Logging\LoggerBase.h"

X_NAMESPACE_BEGIN(core)

class Console;

class LoggerConsoleWritePolicy
{
public:
    explicit LoggerConsoleWritePolicy(const Console& console);

    /// Initializes a custom color table for the console.
    void Init(void);
    void Exit(void);

    void WriteLog(const LoggerBase::Line& line, uint32_t length);
    void WriteWarning(const LoggerBase::Line& line, uint32_t length);
    void WriteError(const LoggerBase::Line& line, uint32_t length);
    void WriteFatal(const LoggerBase::Line& line, uint32_t length);
    void WriteAssert(const LoggerBase::Line& line, uint32_t length);
    void WriteAssertVariable(const LoggerBase::Line& line, uint32_t length);

private:
    HANDLE console_;
    CriticalSection cs_;
};

X_NAMESPACE_END

#endif // X_LOGGERCONSOLEWRITEPOLICY_H_
