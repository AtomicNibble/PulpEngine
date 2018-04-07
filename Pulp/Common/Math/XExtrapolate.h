#pragma once

#ifndef X_MATH_EXTRAPOLATE_H_
#define X_MATH_EXTRAPOLATE_H_

#include <Util\EnumMacros.h>

X_DECLARE_ENUM(ExtrapolationType)
(
    NONE,        // no extrapolation, covered distance = duration * 0.001 * ( baseSpeed )
    LINEAR,      // linear extrapolation, covered distance = duration * 0.001 * ( baseSpeed + speed )
    ACCELLINEAR, // linear acceleration, covered distance = duration * 0.001 * ( baseSpeed + 0.5 * speed )
    DECELLINEAR, // linear deceleration, covered distance = duration * 0.001 * ( baseSpeed + 0.5 * speed )
    ACCELSINE,   // sinusoidal acceleration, covered distance = duration * 0.001 * ( baseSpeed + sqrt( 0.5 ) * speed )
    DECELSINE    // sinusoidal deceleration, covered distance = duration * 0.001 * ( baseSpeed + sqrt( 0.5 ) * speed )
);

template<class type>
class XExtrapolate
{
public:
    XExtrapolate();

    void Init(const int startTime, const int duration, const type& startValue,
        const type& baseSpeed, const type& speed, const ExtrapolationType::Enum ExtrapolationType);
    type GetCurrentValue(int time) const;
    type GetCurrentSpeed(int time) const;
    bool IsDone(int time) const
    {
        return (time >= startTime_ + duration_);
    }
    void SetStartTime(int time)
    {
        startTime_ = time;
    }
    int GetStartTime() const
    {
        return startTime_;
    }
    int GetEndTime() const
    {
        return (duration_ > 0) ? startTime_ + duration_ : 0;
    }
    int GetDuration() const
    {
        return duration_;
    }
    void SetStartValue(const type& value)
    {
        startValue_ = value;
    }
    const type& GetStartValue() const
    {
        return startValue_;
    }
    const type& GetBaseSpeed() const
    {
        return baseSpeed_;
    }
    const type& GetSpeed() const
    {
        return speed_;
    }
    ExtrapolationType::Enum GetExtrapolationType() const
    {
        return extrapolationType_;
    }

private:
    ExtrapolationType::Enum extrapolationType_;
    int startTime_;
    int duration_;
    type startValue_;
    type baseSpeed_;
    type speed_;
};

template<class type>
X_INLINE XExtrapolate<type>::XExtrapolate()
{
    extrapolationType_ = ExtrapolationType::NONE;
    startTime_ = duration_ = 0;

    core::zero_object(startValue_);
    core::zero_object(baseSpeed_);
    core::zero_object(speed_);
}

template<class type>
X_INLINE void XExtrapolate<type>::Init(const int startTime, const int duration,
    const type& startValue, const type& baseSpeed,
    const type& speed, const ExtrapolationType::Enum ExtrapolationType)
{
    this->extrapolationType_ = ExtrapolationType;
    this->startTime_ = startTime;
    this->duration_ = duration;
    this->startValue_ = startValue;
    this->baseSpeed_ = baseSpeed;
    this->speed_ = speed;
}

template<class type>
X_INLINE type XExtrapolate<type>::GetCurrentValue(int time) const
{
    if (time < startTime_) {
        return startValue_;
    }

    if ((time > startTime_ + duration_)) {
        time = startTime_ + duration_;
    }

    switch (extrapolationType_) {
        case ExtrapolationType::NONE: {
            const float deltaTime = (time - startTime_) * 0.001f;
            return startValue_ + deltaTime * baseSpeed_;
        }
        case ExtrapolationType::LINEAR: {
            const float deltaTime = (time - startTime_) * 0.001f;
            return startValue_ + deltaTime * (baseSpeed_ + speed_);
        }
        case ExtrapolationType::ACCELLINEAR: {
            if (duration_ == 0) {
                return startValue_;
            }
            else {
                const float deltaTime = (time - startTime_) / static_cast<float>(duration_);
                const float s = (0.5f * deltaTime * deltaTime) * (static_cast<float>(duration_) * 0.001f);
                return startValue_ + deltaTime * baseSpeed_ + s * speed_;
            }
        }
        case ExtrapolationType::DECELLINEAR: {
            if (duration_ == 0) {
                return startValue_;
            }
            else {
                const float deltaTime = (time - startTime_) / static_cast<float>(duration_);
                const float s = (deltaTime - (0.5f * deltaTime * deltaTime)) * (static_cast<float>(duration_) * 0.001f);
                return startValue_ + deltaTime * baseSpeed_ + s * speed_;
            }
        }
        case ExtrapolationType::ACCELSINE:
            if (duration_ == 0) {
                return startValue_;
            }
            else {
                const float deltaTime = (time - startTime_) / static_cast<float>(duration_);
                const float s = (1.0f - math<float>::cos(deltaTime * math<float>::HALF_PI))
                                * static_cast<float>(duration_) * 0.001f * math<float>::SQRT_1OVER2;
                return startValue_ + deltaTime * baseSpeed_ + s * speed_;
            }

        case ExtrapolationType::DECELSINE:
            if (duration_ == 0) {
                return startValue_;
            }
            else {
                const float deltaTime = (time - startTime_) / static_cast<float>(duration_);
                const float s = math<float>::sin(deltaTime * math<float>::HALF_PI) * static_cast<float>(duration_) * 0.001f * math<float>::SQRT_1OVER2;
                return startValue_ + deltaTime * baseSpeed_ + s * speed_;
            }
    }

    return startValue_;
}

template<class type>
X_INLINE type XExtrapolate<type>::GetCurrentSpeed(int time) const
{
    if (time < startTime_ || duration_ == 0) {
        return (startValue_ - startValue_);
    }

    if ((time > startTime_ + duration_)) {
        return (startValue_ - startValue_);
    }

    const float deltaTime = (time - startTime_) / static_cast<float>(duration_);

    switch (extrapolationType_) {
        case ExtrapolationType::NONE:
            return baseSpeed_;

        case ExtrapolationType::LINEAR:
            return baseSpeed_ + speed_;

        case ExtrapolationType::ACCELLINEAR: {
            const float s = deltaTime;
            return baseSpeed_ + s * speed_;
        }
        case ExtrapolationType::DECELLINEAR: {
            const float s = 1.0f - deltaTime;
            return baseSpeed_ + s * speed_;
        }
        case ExtrapolationType::ACCELSINE: {
            const float s = math<float>::sin(deltaTime * math<float>::HALF_PI);
            return baseSpeed_ + s * speed_;
        }
        case ExtrapolationType::DECELSINE: {
            const float s = math<float>::cos(deltaTime * math<float>::HALF_PI);
            return baseSpeed_ + s * speed_;
        }

        default:
            return baseSpeed_;
    }
}

#endif // !X_MATH_EXTRAPOLATE_H_
