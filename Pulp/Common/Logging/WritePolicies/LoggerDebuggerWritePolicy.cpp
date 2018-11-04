#include "EngineCommon.h"

#include "LoggerDebuggerWritePolicy.h"

X_NAMESPACE_BEGIN(core)

void LoggerDebuggerWritePolicy::Init(void)
{
}

void LoggerDebuggerWritePolicy::Exit(void)
{
}

void LoggerDebuggerWritePolicy::WriteLog(const LoggerBase::Line& line, uint32_t length)
{
    X_UNUSED(length);
    wchar_t wideBuf[sizeof(LoggerBase::Line)];

    OutputDebugStringW(strUtil::Convert(line, wideBuf));
}

void LoggerDebuggerWritePolicy::WriteWarning(const LoggerBase::Line& line, uint32_t length)
{
    X_UNUSED(length);
    wchar_t wideBuf[sizeof(LoggerBase::Line)];

    OutputDebugStringW(strUtil::Convert(line, wideBuf));
}

void LoggerDebuggerWritePolicy::WriteError(const LoggerBase::Line& line, uint32_t length)
{
    X_UNUSED(length);
    wchar_t wideBuf[sizeof(LoggerBase::Line)];

    OutputDebugStringW(strUtil::Convert(line, wideBuf));
}

void LoggerDebuggerWritePolicy::WriteFatal(const LoggerBase::Line& line, uint32_t length)
{
    X_UNUSED(length);
    wchar_t wideBuf[sizeof(LoggerBase::Line)];

    OutputDebugStringW(strUtil::Convert(line, wideBuf));
}

void LoggerDebuggerWritePolicy::WriteAssert(const LoggerBase::Line& line, uint32_t length)
{
    X_UNUSED(length);
    wchar_t wideBuf[sizeof(LoggerBase::Line)];

    OutputDebugStringW(strUtil::Convert(line, wideBuf));
}

void LoggerDebuggerWritePolicy::WriteAssertVariable(const LoggerBase::Line& line, uint32_t length)
{
    X_UNUSED(length);
    wchar_t wideBuf[sizeof(LoggerBase::Line)];

    OutputDebugStringW(strUtil::Convert(line, wideBuf));
}

X_NAMESPACE_END