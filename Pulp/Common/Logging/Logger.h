#pragma once

#ifndef X_LOGGER_H_
#define X_LOGGER_H_

#include "Logging/LoggerBase.h"

X_NAMESPACE_BEGIN(core)

template <class FilterPolicy, class FormatPolicy, class WritePolicy>
class Logger : public LoggerBase
{
public:
	typedef typename FilterPolicy FilterPolicy;
	typedef typename FormatPolicy FormatPolicy;
	typedef typename WritePolicy WritePolicy;

	Logger(void);
	Logger(const FilterPolicy& filter, const FormatPolicy& formatter, const WritePolicy& writer);

	virtual ~Logger(void);

	const FilterPolicy& GetFilterPolicy(void) const;
	const FormatPolicy& GetFormatPolicy(void) const;
	const WritePolicy& GetWritePolicy(void) const;

private:
	virtual void DoLog(const SourceInfo& sourceInfo, const char* channel, size_t verbosity, const char* foramt, va_list args) X_OVERRIDE;
	virtual void DoWarning(const SourceInfo& sourceInfo, const char* channel, const char* foramt, va_list args) X_OVERRIDE;
	virtual void DoError(const SourceInfo& sourceInfo, const char* channel, const char* foramt, va_list args) X_OVERRIDE;
	virtual void DoFatal(const SourceInfo& sourceInfo, const char* channel, const char* foramt, va_list args) X_OVERRIDE;
	virtual void DoAssert(const SourceInfo& sourceInfo, const char* format, va_list args) X_OVERRIDE;
	virtual void DoAssertVariable(const SourceInfo& sourceInfo, const char* format, va_list args) X_OVERRIDE;

	FilterPolicy filter_;
	FormatPolicy formatter_;
	WritePolicy writer_;
};

#include "Logger.inl"

X_NAMESPACE_END


#endif // X_LOGGER_H_
