
X_INLINE TimeVal::TimeVal()
{
    time_ = 0;
}

X_INLINE TimeVal::TimeVal(const float fSeconds)
{
    SetSeconds(fSeconds);
}

X_INLINE TimeVal::TimeVal(const double fSeconds)
{
    SetSeconds(fSeconds);
}

X_INLINE TimeVal::TimeVal(const TimeType inllValue)
{
    time_ = inllValue;
}

X_INLINE TimeVal::TimeVal(const TimeVal& inValue)
{
    time_ = inValue.time_;
}

X_INLINE TimeVal& TimeVal::operator=(const TimeVal& inRhs)
    = default;

X_INLINE float TimeVal::GetSeconds(void) const
{
    return time_ * (1.f / getFreq());
}

X_INLINE float TimeVal::GetDifferenceInSeconds(const TimeVal& startTime) const
{
    return (time_ - startTime.time_) * (1.f / getFreq());
}

X_INLINE void TimeVal::SetSeconds(const float infSec)
{
    time_ = (TimeType)(infSec * getFreq());
}

X_INLINE void TimeVal::SetSeconds(const double infSec)
{
    time_ = (TimeType)(infSec * getFreq());
}

X_INLINE void TimeVal::SetSeconds(const TimeType indwSec)
{
    time_ = indwSec * getFreq();
}

X_INLINE void TimeVal::SetMilliSeconds(const int32_t iniMilliSec)
{
    const float scale = (getFreq() / 1000.f);

    time_ = static_cast<TimeType>(iniMilliSec * scale);
}

X_INLINE void TimeVal::SetMilliSeconds(const double indMilliSec)
{
    const double scale = (getFreq() / 1000.0);

    time_ = static_cast<TimeType>(indMilliSec * scale);
}

X_INLINE void TimeVal::SetMilliSeconds(const TimeType indwMilliSec)
{
    const double scale = (getFreq() / 1000.0);

    time_ = static_cast<TimeType>(indwMilliSec * scale);
}

X_INLINE void TimeVal::SetMicroSeconds(const TimeType indwMicroSec)
{
    const double scale = (getFreq() / (1000.0 * 1000.0));

    time_ = static_cast<TimeType>(indwMicroSec * scale);
}

X_INLINE void TimeVal::SetNanoSeconds(const TimeType indwNanoSec)
{
    const double scale = (getFreq() / (1000.0 * 1000.0 * 1000.0));

    time_ = static_cast<TimeType>(indwNanoSec * scale);
}

// Use only for relative value, absolute values suffer a lot from precision loss.
X_INLINE float TimeVal::GetMilliSeconds(void) const
{
    return time_ * (1000.f / getFreq());
}

X_INLINE TimeVal::TimeType TimeVal::GetMilliSecondsAsInt64(void) const
{
    return time_ * 1000 / getFreq();
}

X_INLINE TimeVal::TimeType TimeVal::GetValue(void) const
{
    return time_;
}

X_INLINE void TimeVal::SetValue(TimeType val)
{
    time_ = val;
}

// Minus.
X_INLINE TimeVal TimeVal::operator-(const TimeVal& inRhs) const
{
    TimeVal ret;
    ret.time_ = time_ - inRhs.time_;
    return ret;
}
// Plus.
X_INLINE TimeVal TimeVal::operator+(const TimeVal& inRhs) const
{
    TimeVal ret;
    ret.time_ = time_ + inRhs.time_;
    return ret;
}

X_INLINE TimeVal TimeVal::operator/(const TimeVal& inRhs) const
{
    TimeVal ret;
    ret.time_ = time_ / inRhs.time_;
    return ret;
}

X_INLINE TimeVal TimeVal::operator%(const TimeVal& inRhs) const
{
    TimeVal ret;
    ret.time_ = time_ % inRhs.time_;
    return ret;
}

// Unary minus.
X_INLINE TimeVal TimeVal::operator-() const
{
    TimeVal ret;
    ret.time_ = -time_;
    return ret;
}

X_INLINE TimeVal& TimeVal::operator+=(const TimeVal& inRhs)
{
    time_ += inRhs.time_;
    return *this;
}

X_INLINE TimeVal& TimeVal::operator-=(const TimeVal& inRhs)
{
    time_ -= inRhs.time_;
    return *this;
}

// comparison
X_INLINE bool TimeVal::operator<(const TimeVal& inRhs) const
{
    return time_ < inRhs.time_;
}

X_INLINE bool TimeVal::operator>(const TimeVal& inRhs) const
{
    return time_ > inRhs.time_;
}

X_INLINE bool TimeVal::operator>=(const TimeVal& inRhs) const
{
    return time_ >= inRhs.time_;
}

X_INLINE bool TimeVal::operator<=(const TimeVal& inRhs) const
{
    return time_ <= inRhs.time_;
}

X_INLINE bool TimeVal::operator==(const TimeVal& inRhs) const
{
    return time_ == inRhs.time_;
}

X_INLINE bool TimeVal::operator!=(const TimeVal& inRhs) const
{
    return time_ != inRhs.time_;
}

X_INLINE core::TimeVal TimeVal::fromMS(TimeType ms)
{
    TimeVal val;
    val.SetMilliSeconds(ms);
    return val;
}

X_INLINE TimeVal::TimeType TimeVal::getFreq(void)
{
    return gEnv->timerFreq;
}