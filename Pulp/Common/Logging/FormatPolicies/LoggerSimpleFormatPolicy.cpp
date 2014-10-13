#include "EngineCommon.h"


#include "LoggerSimpleFormatPolicy.h"
#include "String\StackString.h"

#include <stdio.h>

X_NAMESPACE_BEGIN(core)


/// Empty implementation.
void LoggerSimpleFormatPolicy::Init(void)
{

}

/// Empty implementation.
void LoggerSimpleFormatPolicy::Exit(void)
{

}

/// Formats the given message.
uint32_t LoggerSimpleFormatPolicy::Format(LoggerBase::Line& line, const char* indentation, 
	const char* type, const SourceInfo& sourceInfo, const char* channel,
	size_t verbosity, const char* format, va_list args
	)
{
//	StackString<512> channelStr(channel );
//	channelStr.Append( channel, strUtil::strlen( channel ) );
//	channelStr.Append( "", 1u);
	
	int bytesWritten; 

	bytesWritten = _snprintf_s(line, _TRUNCATE, "%-20s%s", channel, indentation);
	bytesWritten += vsnprintf_s(&line[bytesWritten], sizeof(LoggerBase::Line) - bytesWritten, _TRUNCATE, format, args);
	bytesWritten += _snprintf_s(&line[bytesWritten], sizeof(LoggerBase::Line) - bytesWritten, _TRUNCATE, "\n");

	return safe_static_cast<uint32_t,int>( bytesWritten );
}

 

X_NAMESPACE_END