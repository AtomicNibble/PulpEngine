#include "EngineCommon.h"
#include "LoggerBase.h"

X_NAMESPACE_BEGIN(core)

LoggerBase::LoggerBase(void) :
previous_(nullptr),
next_(nullptr)
{

}

LoggerBase::~LoggerBase(void)
{

}

void LoggerBase::SetParent(ILog* pLog)
{
	pLog_ = pLog;
}

/// ----------------------------------------------------------------------------------

void LoggerBase::Log(X_SOURCE_INFO_LOG_CA(const SourceInfo& sourceInfo) const char* channel,
	int verbosity, const char* format, va_list args)
{
	DoLog(X_SOURCE_INFO_LOG_CA(sourceInfo) channel, verbosity, format, args);
}

void LoggerBase::Warning(X_SOURCE_INFO_LOG_CA(const SourceInfo& sourceInfo) const char* channel,
	const char* format, va_list args)
{
	DoWarning(X_SOURCE_INFO_LOG_CA(sourceInfo) channel, format, args);
}

void LoggerBase::Error(X_SOURCE_INFO_LOG_CA(const SourceInfo& sourceInfo) const char* channel,
	const char* format, va_list args)
{
	DoError(X_SOURCE_INFO_LOG_CA(sourceInfo) channel, format, args);
}

void LoggerBase::Fatal(X_SOURCE_INFO_LOG_CA(const SourceInfo& sourceInfo) const char* channel,
	const char* format, va_list args)
{
	DoFatal(X_SOURCE_INFO_LOG_CA(sourceInfo) channel, format, args);
}

void LoggerBase::Assert(X_SOURCE_INFO_LOG_CA(const SourceInfo& sourceInfo)
	const char* format, va_list args)
{
	DoAssert(X_SOURCE_INFO_LOG_CA(sourceInfo) format, args);
}

void LoggerBase::AssertVariable(X_SOURCE_INFO_LOG_CA(const SourceInfo& sourceInfo)
	const char* format, va_list args)
{
	DoAssertVariable(X_SOURCE_INFO_LOG_CA(sourceInfo) format, args);
}


X_NAMESPACE_END