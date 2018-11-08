#pragma once

#ifndef X_GAMETIMER_H_
#define X_GAMETIMER_H_

#include "Traits\FunctionTraits.h"

X_NAMESPACE_BEGIN(core)

/// \brief high-res timer for genral game use.
/// \details Returns the value as a 64bit int for the following reasons: http://www.altdevblogaday.com/2012/02/05/dont-store-that-in-a-float/
/// ( basically float percision sucks after game running for long time )
namespace SysTimer
{
    typedef core::traits::Function<int64(void)> TimeUpdateFunc;

    void Startup(void);
    void Shutdown(void);

    bool HasFreqChanged(void);

    int64_t Get(void);

    X_INLINE float ToSeconds(int64_t count);
    X_INLINE float ToMilliSeconds(int64_t count);

    X_INLINE int64_t fromSeconds(float value);
    X_INLINE int64_t fromSeconds(double value);
    X_INLINE int64_t fromSeconds(int64_t value);
    X_INLINE int64_t fromMilliSeconds(int32_t value);
    X_INLINE int64_t fromMilliSeconds(float value);
    X_INLINE int64_t fromMilliSeconds(int64_t value);
    X_INLINE int64_t fromMicroSeconds(int64_t value);
    X_INLINE int64_t fromNanoSeconds(int64_t value);

    X_INLINE int64 GetTickPerSec(void);
} // namespace SysTimer

#include "SystemTimer.inl"

X_NAMESPACE_END

#endif // X_GAMETIMER_H_
