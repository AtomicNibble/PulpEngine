#pragma once

#ifndef X_LOGGERNOFILTERPOLICY_H_
#define X_LOGGERNOFILTERPOLICY_H_

X_NAMESPACE_BEGIN(core)

class LoggerNoFilterPolicy
{
public:
    inline void Init(void);

    inline void Exit(void);

    inline void RegisterVars(void);

    inline bool Filter(LogType::Enum type, X_SOURCE_INFO_LOG_CA(const SourceInfo& sourceInfo) const char* channel, int verbosity,
        const char* format, va_list args);
};

#include "LoggerNoFilterPolicy.inl"

X_NAMESPACE_END

#endif // X_LOGGERNOFILTERPOLICY_H_
