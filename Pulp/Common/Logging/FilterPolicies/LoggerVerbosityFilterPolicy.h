#pragma once

#ifndef X_LOGGERVERBOSITYFILTERPOLICY_H_
#define X_LOGGERVERBOSITYFILTERPOLICY_H_

#include <String\StackString.h>

X_NAMESPACE_BEGIN(core)


struct ICVar;

class LoggerVerbosityFilterPolicy
{
public:
    LoggerVerbosityFilterPolicy(int initialVerbosityLvl, const char* nickName);

    void Init(void);
    void Exit(void);

    void RegisterVars(void);

    bool Filter(LogType::Enum type, X_SOURCE_INFO_LOG_CA(const SourceInfo& sourceInfo) const char* channel,
        int verbosity, const char* format, va_list args);

private:
    core::StackString<64> nickName_;
    core::ICVar* pVar_;
    int logVerbosity_;
};

X_NAMESPACE_END

#endif // X_LOGGERVERBOSITYFILTERPOLICY_H_
