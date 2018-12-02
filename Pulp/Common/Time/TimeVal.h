#pragma once

#ifndef _X_TIME_VAL_H_
#define _X_TIME_VAL_H_

X_NAMESPACE_BEGIN(core)

// Holds a time value :)
class TimeVal
{
public:
    typedef int64_t TimeType;

public:
    X_INLINE TimeVal();
    explicit X_INLINE TimeVal(const float fSeconds);
    explicit X_INLINE TimeVal(const double fSeconds);
    //		inllValue - positive negative, absolute or relative in 1 second= PRECISION units.
    explicit X_INLINE TimeVal(const TimeType inllValue);
    X_INLINE TimeVal(const TimeVal& inValue);

    X_INLINE TimeVal& operator=(const TimeVal& inRhs);

    // Use only for relative value, absolute values suffer a lot from precision loss.
    X_INLINE float GetSeconds(void) const;
    // Get relative time difference in seconds - call on the endTime object:  endTime.GetDifferenceInSeconds( startTime );
    X_INLINE float GetDifferenceInSeconds(const TimeVal& startTime) const;
    X_INLINE void SetSeconds(const float infSec);
    X_INLINE void SetSeconds(const double infSec);
    X_INLINE void SetSeconds(const TimeType indwSec);
    X_INLINE void SetMilliSeconds(const int32_t iniMilliSec);
    X_INLINE void SetMilliSeconds(const double indMilliSec);
    X_INLINE void SetMilliSeconds(const TimeType indwMilliSec);
    X_INLINE void SetMicroSeconds(const TimeType indwNanoSec);
    X_INLINE void SetNanoSeconds(const TimeType indwNanoSec);
    // Use only for relative value, absolute values suffer a lot from precision loss.
    X_INLINE float GetMilliSeconds(void) const;
    X_INLINE int32_t GetMilliSecondsAsInt32(void) const;
    X_INLINE TimeType GetMilliSecondsAsInt64(void) const;
    X_INLINE TimeType GetValue(void) const;
    X_INLINE void SetValue(TimeType val);

    X_INLINE TimeVal operator-(const TimeVal& inRhs) const;
    X_INLINE TimeVal operator+(const TimeVal& inRhs) const;
    X_INLINE TimeVal operator/(const TimeVal& inRhs) const;
    X_INLINE TimeVal operator%(const TimeVal& inRhs) const;
    X_INLINE TimeVal operator-() const;
    X_INLINE TimeVal& operator+=(const TimeVal& inRhs);
    X_INLINE TimeVal& operator-=(const TimeVal& inRhs);

    // comparison -----------------------
    X_INLINE bool operator<(const TimeVal& inRhs) const;
    X_INLINE bool operator>(const TimeVal& inRhs) const;
    X_INLINE bool operator>=(const TimeVal& inRhs) const;
    X_INLINE bool operator<=(const TimeVal& inRhs) const;
    X_INLINE bool operator==(const TimeVal& inRhs) const;
    X_INLINE bool operator!=(const TimeVal& inRhs) const;

    // create a time value that contains a given number of ms.
    X_INLINE static core::TimeVal fromMS(TimeType ms);

private:
    X_INLINE static TimeType getFreq(void);

private:
    TimeType time_;
};

#include "TimeVal.inl"

X_NAMESPACE_END

#endif // _X_TIME_VAL_H_