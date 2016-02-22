
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

X_INLINE TimeVal::TimeVal(const TimeVal &inValue)
{
	time_ = inValue.time_;
}

X_INLINE TimeVal::~TimeVal()
{

}


X_INLINE TimeVal& TimeVal::operator=(const TimeVal &inRhs)
{
	time_ = inRhs.time_;
	return *this;
};

X_INLINE float TimeVal::GetSeconds(void) const
{
	return time_ * (1.f / PRECISION);
}

X_INLINE float TimeVal::GetDifferenceInSeconds(const TimeVal& startTime) const
{
	return (time_ - startTime.time_) * (1.f / PRECISION);
}

X_INLINE void TimeVal::SetSeconds(const float infSec)
{
	time_ = (TimeType)(infSec*PRECISION);
}

X_INLINE void TimeVal::SetSeconds(const double infSec)
{
	time_ = (TimeType)(infSec*PRECISION);
}

X_INLINE void TimeVal::SetSeconds(const TimeType indwSec)
{
	time_ = indwSec*PRECISION;
}

X_INLINE void TimeVal::SetMilliSeconds(const int iniMilliSec)
{
	time_ = iniMilliSec*(PRECISION / 1000);
}
X_INLINE void TimeVal::SetMilliSeconds(const double indMilliSec)
{
	time_ = ((TimeType)indMilliSec)*(PRECISION / 1000);
}

X_INLINE void TimeVal::SetMilliSeconds(const TimeType indwMilliSec)
{
	time_ = indwMilliSec*(PRECISION / 1000);
}

X_INLINE void TimeVal::SetNanoSeconds(const TimeType indwNanoSec)
{
	time_ = indwNanoSec / (1000000 / PRECISION);
}

// Use only for relative value, absolute values suffer a lot from precision loss.
X_INLINE float TimeVal::GetMilliSeconds(void) const
{
	return time_ * (1000.f / PRECISION);
}

X_INLINE TimeVal::TimeType TimeVal::GetMilliSecondsAsInt64(void) const
{
	return time_ * 1000 / PRECISION;
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
X_INLINE TimeVal TimeVal::operator-(const TimeVal &inRhs) const 
{ 
	TimeVal ret;	
	ret.time_ = time_ - inRhs.time_; 
	return ret; 
}
// Plus.
X_INLINE TimeVal TimeVal::operator+(const TimeVal &inRhs) const 
{ 
	TimeVal ret;	
	ret.time_ = time_ + inRhs.time_; 
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

X_INLINE TimeVal& TimeVal::operator/=(int inRhs) 
{ 
	time_ /= inRhs; 
	return *this; 
}

// comparison 
X_INLINE bool TimeVal::operator<(const TimeVal &inRhs) const
{ 
	return time_ < inRhs.time_; 
}

X_INLINE bool TimeVal::operator>(const TimeVal &inRhs) const 
{ 
	return time_ > inRhs.time_;
}

X_INLINE bool TimeVal::operator>=(const TimeVal &inRhs) const 
{ 
	return time_ >= inRhs.time_; 
}

X_INLINE bool TimeVal::operator<=(const TimeVal &inRhs) const 
{ 
	return time_ <= inRhs.time_; 
}

X_INLINE bool TimeVal::operator==(const TimeVal &inRhs) const 
{ 
	return time_ == inRhs.time_; 
}

X_INLINE bool TimeVal::operator!=(const TimeVal &inRhs) const 
{ 
	return time_ != inRhs.time_; 
}

X_INLINE core::TimeVal TimeVal::fromMS(TimeType ms)
{
	TimeVal val;
	val.SetMilliSeconds(ms);
	return val;
}