#include "EngineCommon.h"

#include "Logging\LoggerBase.h"
#include "LoggerExtendedFormatPolicy.h"
#include <stdio.h>

X_NAMESPACE_BEGIN(core)


/// Empty implementation.
void LoggerExtendedFormatPolicy::Init(void)
{

}

/// Empty implementation.
void LoggerExtendedFormatPolicy::Exit(void)
{

}

/// Formats the given message.
uint32_t LoggerExtendedFormatPolicy::Format(LoggerBase::Line& line, const char* indentation, 
	const char* type, const SourceInfo& sourceInfo, const char* channel, size_t verbosity, const char* format, va_list args)
{
	X_UNUSED(type);
	X_UNUSED(verbosity);

	int bytesWritten; 

	bytesWritten = _snprintf_s(line, _TRUNCATE, "%s(%d): [%s:%s] %s",  
		sourceInfo.file_, sourceInfo.line_, sourceInfo.module_, channel, 
	//	type, verbosity, 
		indentation
		);

	bytesWritten += vsnprintf_s(&line[bytesWritten], sizeof(LoggerBase::Line) - bytesWritten, _TRUNCATE, format, args);
	bytesWritten += _snprintf_s(&line[bytesWritten], sizeof(LoggerBase::Line) - bytesWritten, _TRUNCATE, "\n");

	return safe_static_cast<int,uint32_t>( bytesWritten );
}



X_NAMESPACE_END