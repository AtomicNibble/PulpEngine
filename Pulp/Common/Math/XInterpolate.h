#pragma once

#ifndef X_MATH_INTERPOLATE_H_
#define X_MATH_INTERPOLATE_H_

template<class type>
class XInterpolate
{
public:
    XInterpolate();

    void Init(const int startTime, const int duration,
        const type& startValue, const type& endValue);
    void SetStartTime(int time)
    {
        this->startTime_ = time;
    }
    void SetDuration(int duration)
    {
        this->duration_ = duration;
    }
    void SetStartValue(const type& startValue)
    {
        this->startValue_ = startValue;
    }
    void SetEndValue(const type& endValue)
    {
        this->endValue_ = endValue;
    }

    type GetCurrentValue(int time) const;
    bool IsDone(int time) const
    {
        return (time >= startTime_ + duration_);
    }

    int GetStartTime() const
    {
        return startTime_;
    }
    int GetEndTime() const
    {
        return startTime_ + duration_;
    }
    int GetDuration() const
    {
        return duration_;
    }
    const type& GetStartValue() const
    {
        return startValue_;
    }
    const type& GetEndValue() const
    {
        return endValue_;
    }

private:
    int startTime_;
    int duration_;
    type startValue_;
    type endValue_;
};

template<class type>
class XInterpolateAccelDecelLinear
{
public:
    XInterpolateAccelDecelLinear();

    void Init(const int startTime, const int accelTime, const int decelTime,
        const int duration, const type& startValue, const type& endValue);
    void SetStartTime(int time)
    {
        startTime_ = time;
        Invalidate();
    }
    void SetStartValue(const type& startValue)
    {
        this->startValue_ = startValue;
        Invalidate();
    }
    void SetEndValue(const type& endValue)
    {
        this->endValue_ = endValue;
        Invalidate();
    }

    type GetCurrentValue(int time) const;
    type GetCurrentSpeed(int time) const;
    bool IsDone(int time) const
    {
        return (time >= startTime_ + accelTime_ + linearTime_ + decelTime_);
    }

    int GetStartTime() const
    {
        return startTime_;
    }
    int GetEndTime() const
    {
        return startTime_ + accelTime_ + linearTime_ + decelTime_;
    }
    int GetDuration() const
    {
        return accelTime_ + linearTime_ + decelTime_;
    }
    int GetAcceleration() const
    {
        return accelTime_;
    }
    int GetDeceleration() const
    {
        return decelTime_;
    }
    const type& GetStartValue() const
    {
        return startValue_;
    }
    const type& GetEndValue() const
    {
        return endValue_;
    }

private:
    int startTime_;
    int accelTime_;
    int linearTime_;
    int decelTime_;
    type startValue_;
    type endValue_;
    mutable XExtrapolate<type> extrapolate_;

    void Invalidate();
    void SetPhase(int time) const;
};

template<class type>
class XInterpolateAccelDecelSine
{
public:
    XInterpolateAccelDecelSine();

    void Init(const int startTime, const int accelTime,
        const int decelTime, const int duration,
        const type& startValue, const type& endValue);
    void SetStartTime(int time)
    {
        startTime_ = time;
        Invalidate();
    }
    void SetStartValue(const type& startValue)
    {
        this->startValue_ = startValue;
        Invalidate();
    }
    void SetEndValue(const type& endValue)
    {
        this->endValue_ = endValue;
        Invalidate();
    }

    type GetCurrentValue(int time) const;
    type GetCurrentSpeed(int time) const;
    bool IsDone(int time) const
    {
        return (time >= startTime_ + accelTime_ + linearTime_ + decelTime_);
    }

    int GetStartTime() const
    {
        return startTime_;
    }
    int GetEndTime() const
    {
        return startTime_ + accelTime_ + linearTime_ + decelTime_;
    }
    int GetDuration() const
    {
        return accelTime_ + linearTime_ + decelTime_;
    }
    int GetAcceleration() const
    {
        return accelTime_;
    }
    int GetDeceleration() const
    {
        return decelTime_;
    }
    const type& GetStartValue() const
    {
        return startValue_;
    }
    const type& GetEndValue() const
    {
        return endValue_;
    }

private:
    int startTime_;
    int accelTime_;
    int linearTime_;
    int decelTime_;
    type startValue_;
    type endValue_;
    mutable XExtrapolate<type> extrapolate_;

    void Invalidate();
    void SetPhase(int time) const;
};

#include "XInterpolate.inl"

#endif // !X_MATH_INTERPOLATE_H_
