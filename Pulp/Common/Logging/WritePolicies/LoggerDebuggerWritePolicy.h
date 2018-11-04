#pragma once

#ifndef X_LOGGERDEBUGGERWRITEPOLICY_H_
#define X_LOGGERDEBUGGERWRITEPOLICY_H_

#include "Logging\LoggerBase.h"

X_NAMESPACE_BEGIN(core)

class LoggerDebuggerWritePolicy
{
public:
    void Init(void);
    void Exit(void);

    void WriteLog(const LoggerBase::Line& line, uint32_t length);
    void WriteWarning(const LoggerBase::Line& line, uint32_t length);
    void WriteError(const LoggerBase::Line& line, uint32_t length);
    void WriteFatal(const LoggerBase::Line& line, uint32_t length);
    void WriteAssert(const LoggerBase::Line& line, uint32_t length);
    void WriteAssertVariable(const LoggerBase::Line& line, uint32_t length);
};

X_NAMESPACE_END

#endif // X_LOGGERDEBUGGERWRITEPOLICY_H_
