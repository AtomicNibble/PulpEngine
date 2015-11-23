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

void LoggerBase::Log(const SourceInfo& sourceInfo, const char* channel,
	size_t verbosity, const char* format, va_list args)
{
	DoLog(sourceInfo, channel, verbosity, format, args);
}

void LoggerBase::Warning(const SourceInfo& sourceInfo, const char* channel,
	const char* format, va_list args)
{
	DoWarning(sourceInfo, channel, format, args);
}

void LoggerBase::Error(const SourceInfo& sourceInfo, const char* channel,
	const char* format, va_list args)
{
	DoError(sourceInfo, channel, format, args);
}

void LoggerBase::Fatal(const SourceInfo& sourceInfo, const char* channel,
	const char* format, va_list args)
{
	DoFatal(sourceInfo, channel, format, args);
}

void LoggerBase::Assert(const SourceInfo& sourceInfo, 
	const char* format, va_list args)
{
	DoAssert(sourceInfo, format, args);
}

void LoggerBase::AssertVariable(const SourceInfo& sourceInfo, 
	const char* format, va_list args)
{
	DoAssertVariable(sourceInfo, format, args);
}


/// ----------------------------------------------------------------------------------


void LoggerBase::DoLog(const SourceInfo& sourceInfo, const char* channel,
	size_t verbosity, const char* format, va_list args)
{
	X_UNUSED(sourceInfo);
	X_UNUSED(channel);
	X_UNUSED(verbosity);
	X_UNUSED(format);
	X_UNUSED(args);
}

void LoggerBase::DoWarning(const SourceInfo& sourceInfo, const char* channel,
	const char* format, va_list args)
{
	X_UNUSED(sourceInfo);
	X_UNUSED(channel);
	X_UNUSED(format);
	X_UNUSED(args);
}

void LoggerBase::DoError(const SourceInfo& sourceInfo, const char* channel,
	const char* format, va_list args)
{
	X_UNUSED(sourceInfo);
	X_UNUSED(channel);
	X_UNUSED(format);
	X_UNUSED(args);
}

void LoggerBase::DoFatal(const SourceInfo& sourceInfo, const char* channel,
	const char* format, va_list args)
{
	X_UNUSED(sourceInfo);
	X_UNUSED(channel);
	X_UNUSED(format);
	X_UNUSED(args);
}

void LoggerBase::DoAssert(const SourceInfo& sourceInfo, const char* format, 
	va_list args)
{
	X_UNUSED(sourceInfo);
	X_UNUSED(format);
	X_UNUSED(args);
}

void LoggerBase::DoAssertVariable(const SourceInfo& sourceInfo, 
	const char* format, va_list args)
{
	X_UNUSED(sourceInfo);
	X_UNUSED(format);
	X_UNUSED(args);
}






X_NAMESPACE_END