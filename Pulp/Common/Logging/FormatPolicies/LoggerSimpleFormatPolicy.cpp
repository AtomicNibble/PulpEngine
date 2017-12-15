#include "EngineCommon.h"


#include "LoggerSimpleFormatPolicy.h"

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
	LogType::Enum type, X_SOURCE_INFO_LOG_CA(const SourceInfo&)
	const char* channel, size_t verbosity, const char* format, va_list args)
{
	X_UNUSED(type, verbosity);
	
	int32_t bytesWritten;
	bytesWritten = _snprintf_s(line, _TRUNCATE, "%-20s| %s", channel, indentation);
	bytesWritten += vsnprintf_s(&line[bytesWritten], sizeof(LoggerBase::Line) - bytesWritten, _TRUNCATE, format, args);
	bytesWritten += _snprintf_s(&line[bytesWritten], sizeof(LoggerBase::Line) - bytesWritten, _TRUNCATE, "\n");

	return safe_static_cast<uint32_t, int32_t>( bytesWritten );
}

 // ------------------------------------------------


void LoggerSimpleFormatPolicyStripColors::Init(void)
{

}

void LoggerSimpleFormatPolicyStripColors::Exit(void)
{

}

uint32_t LoggerSimpleFormatPolicyStripColors::Format(LoggerBase::Line& line, const char* indentation,
	LogType::Enum type, X_SOURCE_INFO_LOG_CA(const SourceInfo& sourceInfo)
	const char* channel, size_t verbosity, const char* format, va_list args)
{
	int32_t bytesWritten = LoggerSimpleFormatPolicy::Format(line, indentation, type, X_SOURCE_INFO_LOG_CA(sourceInfo)
																channel, verbosity, format, args);

	// this not so bad as the buffer should already be in the cache.
	int32_t len = bytesWritten;
	for (int32_t i = 0; i < len; i++)
	{
		if (line[i] == '^' && (i + 1) < len && core::strUtil::IsDigit(line[i + 1]))
		{
			// we remove both ^ and col.
			const int32_t bytesToCopy = bytesWritten - i;
			std::memmove(&line[i], &line[i + 2], bytesToCopy);

			len -= 2;
			bytesWritten -= 2;
		}
	}

	return safe_static_cast<uint32_t, int32_t>(bytesWritten);
}


X_NAMESPACE_END