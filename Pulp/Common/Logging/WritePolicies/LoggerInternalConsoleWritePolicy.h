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

    void Init(void);
    void Exit(void);

    void WriteLog(const LoggerBase::Line& line, uint32_t length);
    void WriteWarning(const LoggerBase::Line& line, uint32_t length);
    void WriteError(const LoggerBase::Line& line, uint32_t length);
    void WriteFatal(const LoggerBase::Line& line, uint32_t length);
    void WriteAssert(const LoggerBase::Line& line, uint32_t length);
    void WriteAssertVariable(const LoggerBase::Line& line, uint32_t length);

private:
    ICore* pCore_;
};

X_NAMESPACE_END

#endif // !X_LOGGER_INTERNAL_CONSOLE_WRITE_POLICY_H_
