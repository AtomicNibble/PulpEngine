#include "stdafx.h"
#include "NullLog.h"


X_NAMESPACE_BEGIN(core)

void NullLog::Init(void)
{

}

void NullLog::ShutDown(void)
{

}

void NullLog::Log(X_SOURCE_INFO_LOG_CA(const SourceInfo&) const char* channel, int verbosity, const char* format, ...)
{
    X_UNUSED(channel, verbosity, format);
}

void NullLog::Warning(X_SOURCE_INFO_LOG_CA(const SourceInfo&) const char* channel, const char* format, ...)
{
    X_UNUSED(channel, format);
}

void NullLog::Error(X_SOURCE_INFO_LOG_CA(const SourceInfo&) const char* channel, const char* format, ...)
{
    X_UNUSED(channel, format);
}

void NullLog::Fatal(X_SOURCE_INFO_LOG_CA(const SourceInfo&) const char* channel, const char* format, ...)
{
    X_UNUSED(channel, format);
}

void NullLog::Assert(X_SOURCE_INFO_LOG_CA(const SourceInfo&) const char* format, va_list args)
{
    X_UNUSED(format, args);
}

void NullLog::AssertVariable(X_SOURCE_INFO_LOG_CA(const SourceInfo&) const char* format, ...)
{
    X_UNUSED(format);
}

void NullLog::AddLogger(LoggerBase* logger)
{
    X_UNUSED(logger);
}

void NullLog::RemoveLogger(LoggerBase* logger)
{
    X_UNUSED(logger);
}

const char* NullLog::GetIndentation(void)
{
    return "";
}

void NullLog::Indent(void)
{
}

void NullLog::UnIndent(void)
{
}


X_NAMESPACE_END