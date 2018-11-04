#include "EngineCommon.h"
#include "LoggerInternalConsoleWritePolicy.h"

X_NAMESPACE_BEGIN(core)

LoggerInternalConsoleWritePolicy::LoggerInternalConsoleWritePolicy()
{
}

void LoggerInternalConsoleWritePolicy::Init(void)
{
    X_ASSERT_NOT_NULL(gEnv);
    X_ASSERT_NOT_NULL(gEnv->pCore);
    pCore_ = gEnv->pCore;
}

void LoggerInternalConsoleWritePolicy::Exit(void)
{
}

void LoggerInternalConsoleWritePolicy::WriteLog(const LoggerBase::Line& line, uint32_t length)
{
    pCore_->GetIConsole()->addLineToLog(line, length);
}

void LoggerInternalConsoleWritePolicy::WriteWarning(const LoggerBase::Line& line, uint32_t length)
{
    pCore_->GetIConsole()->addLineToLog(line, length);
}

void LoggerInternalConsoleWritePolicy::WriteError(const LoggerBase::Line& line, uint32_t length)
{
    pCore_->GetIConsole()->addLineToLog(line, length);
}

void LoggerInternalConsoleWritePolicy::WriteFatal(const LoggerBase::Line& line, uint32_t length)
{
    pCore_->GetIConsole()->addLineToLog(line, length);
}

void LoggerInternalConsoleWritePolicy::WriteAssert(const LoggerBase::Line& line, uint32_t length)
{
    pCore_->GetIConsole()->addLineToLog(line, length);
}

void LoggerInternalConsoleWritePolicy::WriteAssertVariable(const LoggerBase::Line& line, uint32_t length)
{
    pCore_->GetIConsole()->addLineToLog(line, length);
}

X_NAMESPACE_END