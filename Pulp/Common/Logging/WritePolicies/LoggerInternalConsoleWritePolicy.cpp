#include "EngineCommon.h"
#include "LoggerInternalConsoleWritePolicy.h"

X_NAMESPACE_BEGIN(core)

LoggerInternalConsoleWritePolicy::LoggerInternalConsoleWritePolicy()
{
}

/// Empty implementation.
void LoggerInternalConsoleWritePolicy::Init(void)
{
    X_ASSERT_NOT_NULL(gEnv);
    X_ASSERT_NOT_NULL(gEnv->pCore);
    pCore_ = gEnv->pCore;
}

/// Empty implementation.
void LoggerInternalConsoleWritePolicy::Exit(void)
{
}

/// Writes a log message to the console.
void LoggerInternalConsoleWritePolicy::WriteLog(const LoggerBase::Line& line, uint32_t length)
{
    pCore_->GetIConsole()->addLineToLog(line, length);
}

/// Writes a warning message to the console.
void LoggerInternalConsoleWritePolicy::WriteWarning(const LoggerBase::Line& line, uint32_t length)
{
    pCore_->GetIConsole()->addLineToLog(line, length);
}

/// Writes an error message to the console.
void LoggerInternalConsoleWritePolicy::WriteError(const LoggerBase::Line& line, uint32_t length)
{
    pCore_->GetIConsole()->addLineToLog(line, length);
}

/// Writes a fatal error message to the console.
void LoggerInternalConsoleWritePolicy::WriteFatal(const LoggerBase::Line& line, uint32_t length)
{
    pCore_->GetIConsole()->addLineToLog(line, length);
}

/// Writes an assert message to the console.
void LoggerInternalConsoleWritePolicy::WriteAssert(const LoggerBase::Line& line, uint32_t length)
{
    pCore_->GetIConsole()->addLineToLog(line, length);
}

/// Writes an assert variable message to the console.
void LoggerInternalConsoleWritePolicy::WriteAssertVariable(const LoggerBase::Line& line, uint32_t length)
{
    pCore_->GetIConsole()->addLineToLog(line, length);
}

X_NAMESPACE_END