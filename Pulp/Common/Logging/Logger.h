#pragma once

#ifndef X_LOGGER_H_
#define X_LOGGER_H_

#include "Logging/LoggerBase.h"

X_NAMESPACE_BEGIN(core)

template<class FilterPolicy, class FormatPolicy, class WritePolicy>
class Logger : public LoggerBase
{
public:
    typedef FilterPolicy FilterPolicy;
    typedef FormatPolicy FormatPolicy;
    typedef WritePolicy WritePolicy;

    Logger(void);
    Logger(const FilterPolicy& filter, const FormatPolicy& formatter, const WritePolicy& writer);

    virtual ~Logger(void) X_FINAL;

    const FilterPolicy& GetFilterPolicy(void) const;
    const FormatPolicy& GetFormatPolicy(void) const;
    const WritePolicy& GetWritePolicy(void) const;

    FilterPolicy& GetFilterPolicy(void);
    FormatPolicy& GetFormatPolicy(void);
    WritePolicy& GetWritePolicy(void);

private:
    virtual void DoLog(X_SOURCE_INFO_LOG_CA(const SourceInfo& sourceInfo) const char* channel, int verbosity, const char* foramt, va_list args) X_FINAL;
    virtual void DoWarning(X_SOURCE_INFO_LOG_CA(const SourceInfo& sourceInfo) const char* channel, const char* foramt, va_list args) X_FINAL;
    virtual void DoError(X_SOURCE_INFO_LOG_CA(const SourceInfo& sourceInfo) const char* channel, const char* foramt, va_list args) X_FINAL;
    virtual void DoFatal(X_SOURCE_INFO_LOG_CA(const SourceInfo& sourceInfo) const char* channel, const char* foramt, va_list args) X_FINAL;
    virtual void DoAssert(X_SOURCE_INFO_LOG_CA(const SourceInfo& sourceInfo) const char* format, va_list args) X_FINAL;
    virtual void DoAssertVariable(X_SOURCE_INFO_LOG_CA(const SourceInfo& sourceInfo) const char* format, va_list args) X_FINAL;

    FilterPolicy filter_;
    FormatPolicy formatter_;
    WritePolicy writer_;
};

#include "Logger.inl"

X_NAMESPACE_END

#endif // X_LOGGER_H_
