

template<class type>
X_INLINE XInterpolate<type>::XInterpolate()
{
    startTime_ = duration_ = 0;

    core::zero_object(startValue_);
    core::zero_object(endValue_);
}

template<class type>
X_INLINE void XInterpolate<type>::Init(const int startTime,
    const int duration, const type& startValue, const type& endValue)
{
    this->startTime_ = startTime;
    this->duration_ = duration;
    this->startValue_ = startValue;
    this->endValue_ = endValue;
}

template<class type>
X_INLINE type XInterpolate<type>::GetCurrentValue(int time) const
{
    if (time <= startTime_) {
        return startValue_;
    }
    else if (time >= startTime_ + duration_) {
        return endValue_;
    }
    else {
        const float deltaTime = time - startTime_;
        const float f = deltaTime / (float)duration_;
        const type range = (endValue_ - startValue_);
        return startValue_ + (range * f);
    }
}

template<class type>
X_INLINE XInterpolateAccelDecelLinear<type>::XInterpolateAccelDecelLinear()
{
    startTime_ = accelTime_ = linearTime_ = decelTime_ = 0;
    core::zero_object(startValue_);
    endValue_ = startValue_;
}

template<class type>
X_INLINE void XInterpolateAccelDecelLinear<type>::Init(const int startTime,
    const int accelTime, const int decelTime,
    const int duration, const type& startValue, const type& endValue)
{
    this->startTime_ = startTime;
    this->accelTime_ = accelTime;
    this->decelTime_ = decelTime;
    this->startValue_ = startValue;
    this->endValue_ = endValue;

    if (duration <= 0) {
        return;
    }

    {
        const int transTime = accelTime_ + decelTime_;

        if (transTime > duration && transTime > 0) {
            this->accelTime_ = this->accelTime_ * duration / transTime;
            this->decelTime_ = duration - this->accelTime_;
        }
    }

    this->linearTime_ = duration - this->accelTime_ - this->decelTime_;
    const type speed = (endValue_ - startValue_) * (1000.0f / ((float)this->linearTime_ + (this->accelTime_ + this->decelTime_) * 0.5f));

    if (this->accelTime_) {
        extrapolate_.Init(startTime_, this->accelTime_, startValue_,
            (startValue_ - startValue_), speed, extrapolationType::ACCELLINEAR);
    }
    else if (this->linearTime_) {
        extrapolate_.Init(startTime_, this->linearTime_, startValue_,
            (startValue_ - startValue_), speed, extrapolationType::LINEAR);
    }
    else {
        extrapolate_.Init(startTime_, this->decelTime_, startValue_,
            (startValue_ - startValue_), speed, extrapolationType::DECELLINEAR);
    }
}

template<class type>
X_INLINE void XInterpolateAccelDecelLinear<type>::Invalidate()
{
    extrapolate_.Init(0, 0, extrapolate_.GetStartValue(), extrapolate_.GetBaseSpeed(), extrapolate_.GetSpeed(), EXTRAPOLATION_NONE);
}

template<class type>
X_INLINE void XInterpolateAccelDecelLinear<type>::SetPhase(int time) const
{
    const float deltaTime = static_cast<float>(time - startTime_);

    if (deltaTime < accelTime_) {
        if (extrapolate_.GetExtrapolationType() != extrapolationType::ACCELLINEAR) {
            extrapolate_.Init(startTime_, accelTime_, startValue_,
                extrapolate_.GetBaseSpeed(), extrapolate_.GetSpeed(), extrapolationType::ACCELLINEAR);
        }
    }
    else if (deltaTime < accelTime_ + linearTime_) {
        if (extrapolate_.GetExtrapolationType() != extrapolationType::LINEAR) {
            extrapolate_.Init(startTime_ + accelTime_, linearTime_,
                startValue_ + extrapolate_.GetSpeed() * (accelTime_ * 0.001f * 0.5f),
                extrapolate_.GetBaseSpeed(), extrapolate_.GetSpeed(), extrapolationType::LINEAR);
        }
    }
    else {
        if (extrapolate_.GetExtrapolationType() != extrapolationType::DECELLINEAR) {
            extrapolate_.Init(startTime_ + accelTime_ + linearTime_, decelTime_,
                endValue_ - (extrapolate_.GetSpeed() * (decelTime_ * 0.001f * 0.5f)),
                extrapolate_.GetBaseSpeed(), extrapolate_.GetSpeed(), extrapolationType::DECELLINEAR);
        }
    }
}

template<class type>
X_INLINE type XInterpolateAccelDecelLinear<type>::GetCurrentValue(int time) const
{
    SetPhase(time);
    return extrapolate_.GetCurrentValue(time);
}

template<class type>
X_INLINE type XInterpolateAccelDecelLinear<type>::GetCurrentSpeed(int time) const
{
    SetPhase(time);
    return extrapolate_.GetCurrentSpeed(time);
}

template<class type>
X_INLINE XInterpolateAccelDecelSine<type>::XInterpolateAccelDecelSine()
{
    startTime_ = accelTime_ = linearTime_ = decelTime_ = 0;

    core::zero_object(startValue_);
    core::zero_object(endValue_);
}

template<class type>
X_INLINE void XInterpolateAccelDecelSine<type>::Init(const int startTime,
    const int accelTime, const int decelTime,
    const int duration, const type& startValue, const type& endValue)
{
    this->startTime_ = startTime;
    this->accelTime_ = accelTime;
    this->decelTime_ = decelTime;
    this->startValue_ = startValue;
    this->endValue_ = endValue;

    if (duration_ <= 0) {
        return;
    }

    if (this->accelTime_ + this->decelTime_ > duration_) {
        this->accelTime_ = this->accelTime_ * duration_ / (this->accelTime_ + this->decelTime_);
        this->decelTime_ = duration_ - this->accelTime_;
    }
    this->linearTime_ = duration_ - this->accelTime_ - this->decelTime_;
    const type speed = (endValue_ - startValue_) * (1000.0f / ((float)this->linearTime_ + (this->accelTime_ + this->decelTime_) * idMath::SQRT_1OVER2));

    if (this->accelTime_) {
        extrapolate_.Init(startTime_, this->accelTime_, startValue_, (startValue_ - startValue_), speed, EXTRAPOLATION_ACCELSINE); //-V501
    }
    else if (this->linearTime_) {
        extrapolate_.Init(startTime_, this->linearTime_, startValue_, (startValue_ - startValue_), speed, EXTRAPOLATION_LINEAR); //-V501
    }
    else {
        extrapolate_.Init(startTime_, this->decelTime_, startValue_, (startValue_ - startValue_), speed, EXTRAPOLATION_DECELSINE); //-V501
    }
}

template<class type>
X_INLINE void XInterpolateAccelDecelSine<type>::Invalidate()
{
    extrapolate_.Init(0, 0, extrapolate_.GetStartValue(), extrapolate_.GetBaseSpeed(), extrapolate_.GetSpeed(), EXTRAPOLATION_NONE);
}

template<class type>
X_INLINE void XInterpolateAccelDecelSine<type>::SetPhase(int time) const
{
    const float deltaTime = time - startTime_;
    if (deltaTime < accelTime_) {
        if (extrapolate_.GetExtrapolationType() != EXTRAPOLATION_ACCELSINE) {
            extrapolate_.Init(startTime_, accelTime_, startValue_, extrapolate_.GetBaseSpeed(), extrapolate_.GetSpeed(), EXTRAPOLATION_ACCELSINE);
        }
    }
    else if (deltaTime < accelTime_ + linearTime_) {
        if (extrapolate_.GetExtrapolationType() != EXTRAPOLATION_LINEAR) {
            extrapolate_.Init(startTime_ + accelTime_, linearTime_, startValue_ + extrapolate_.GetSpeed() * (accelTime_ * 0.001f * idMath::SQRT_1OVER2), extrapolate_.GetBaseSpeed(), extrapolate_.GetSpeed(), EXTRAPOLATION_LINEAR);
        }
    }
    else {
        if (extrapolate_.GetExtrapolationType() != EXTRAPOLATION_DECELSINE) {
            extrapolate_.Init(startTime_ + accelTime_ + linearTime_, decelTime_, endValue_ - (extrapolate_.GetSpeed() * (decelTime_ * 0.001f * idMath::SQRT_1OVER2)), extrapolate_.GetBaseSpeed(), extrapolate_.GetSpeed(), EXTRAPOLATION_DECELSINE);
        }
    }
}

template<class type>
X_INLINE type XInterpolateAccelDecelSine<type>::GetCurrentValue(int time) const
{
    SetPhase(time);
    return extrapolate_.GetCurrentValue(time);
}

template<class type>
X_INLINE type XInterpolateAccelDecelSine<type>::GetCurrentSpeed(int time) const
{
    SetPhase(time);
    return extrapolate_.GetCurrentSpeed(time);
}