#pragma once


#include "Logging\LoggerBase.h"

X_NAMESPACE_BEGIN(maya)

class LoggerMayaFormatPolicy
{
public:
	/// Empty implementation.
	void Init(void);

	/// Empty implementation.
	void Exit(void);

	/// Formats the given message.
	uint32_t Format(core::LoggerBase::Line& line, const char* indentation, const char* type,
		X_SOURCE_INFO_LOG_CA(const SourceInfo& sourceInfo) const char* channel, size_t verbosity,
		const char* format, va_list args);
};

X_NAMESPACE_END