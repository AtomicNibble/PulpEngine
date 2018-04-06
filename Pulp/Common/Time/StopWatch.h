#pragma once

#include "Time\TimeVal.h"

X_NAMESPACE_BEGIN(core)

/// \class StopWatch
/// \brief Can be used to measure elpased time since the time was constructed / started
/// Makes use of the game timer providing high-resolution
class StopWatch
{
public:
    X_INLINE StopWatch(void);

    X_INLINE void Start(void);

    X_INLINE int64_t GetCount(void) const;

    X_INLINE float GetSeconds(void) const;
    X_INLINE float GetMilliSeconds(void) const;
    X_INLINE TimeVal GetTimeVal(void) const;
    X_INLINE TimeVal GetStart(void) const;
    X_INLINE TimeVal GetEnd(void) const;

    static TimeVal GetTimeNow(void);

private:
    static int64_t SysGet(void);
    static float SysToSeconds(int64_t count);
    static float SysToMilliSeconds(int64_t count);

private:
    int64_t start_;
};

#include "StopWatch.inl"

X_NAMESPACE_END
