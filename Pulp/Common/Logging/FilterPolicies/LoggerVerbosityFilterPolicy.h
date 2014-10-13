#pragma once

#ifndef X_LOGGERVERBOSITYFILTERPOLICY_H_
#define X_LOGGERVERBOSITYFILTERPOLICY_H_


X_NAMESPACE_BEGIN(core)

/// \ingroup Logging
/// \brief A class that implements a filter policy for log messages.
/// \details This class implements the concepts of a filter policy as expected by the Logger class. It filters
/// messages based on the currently set verbosity level.
/// 
/// The verbosity level is a single xSettingInt named "core_logVerbosity" that has a range of [0, 2].
/// \sa X_LOG0 X_LOG1 X_LOG2 X_WARNING X_ERROR X_FATAL Logger LoggerBase
class LoggerVerbosityFilterPolicy
{
public:
	/// Empty implementation.
	void Init(void);

	/// Empty implementation.
	void Exit(void);

	/// Filters messages according to the current verbosity level.
	bool Filter(const char* type, const SourceInfo& sourceInfo, const char* channel, int verbosity, const char* format, va_list args);
};

X_NAMESPACE_END


#endif // X_LOGGERVERBOSITYFILTERPOLICY_H_
