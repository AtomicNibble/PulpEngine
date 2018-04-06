#pragma once

#include "Threading/CriticalSection.h"

#include "Logging\LoggerBase.h"

X_NAMESPACE_BEGIN(maya)

class LoggerMayaWritePolicy
{
public:
    LoggerMayaWritePolicy() = default;
    ~LoggerMayaWritePolicy() = default;

    void Init(void);

    /// Empty implementation.
    void Exit(void);

    /// Writes a log message to the debugger.
    void WriteLog(const core::LoggerBase::Line& line, uint32_t length);

    /// Writes a warning message to the debugger.
    void WriteWarning(const core::LoggerBase::Line& line, uint32_t length);

    /// Writes an error message to the debugger.
    void WriteError(const core::LoggerBase::Line& line, uint32_t length);

    /// Writes a fatal error message to the debugger.
    void WriteFatal(const core::LoggerBase::Line& line, uint32_t length);

    /// Writes an assert message to the debugger.
    void WriteAssert(const core::LoggerBase::Line& line, uint32_t length);

    /// Writes an assert variable message to the debugger.
    void WriteAssertVariable(const core::LoggerBase::Line& line, uint32_t length);
};

X_NAMESPACE_END
