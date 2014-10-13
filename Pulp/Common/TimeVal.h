#pragma once

#ifndef _X_TIME_VAL_H_
#define _X_TIME_VAL_H_

X_NAMESPACE_BEGIN(core)

// Holds a time value :)
class TimeVal
{
public:
	typedef int64_t TimeType;

	X_INLINE TimeVal()
	{
		time_ = 0;
	}

	// Constructor.
	X_INLINE TimeVal(const float fSeconds)
	{
		SetSeconds(fSeconds);
	}

	X_INLINE TimeVal(const double fSeconds)
	{
		SetSeconds(fSeconds);
	}

	// Constructor.
	// Arguments:
	//		inllValue - positive negative, absolute or relative in 1 second= PRECISION units.
	X_INLINE TimeVal(const TimeType inllValue)
	{
		time_ = inllValue;
	}

	// Copy constructor.
	X_INLINE TimeVal(const TimeVal &inValue)
	{
		time_ = inValue.time_;
	}

	// Destructor.
	X_INLINE ~TimeVal() {}

	// Description:
	//		Assignment operator.
	// Arguments:
	//		TimeVal - Right side.
	X_INLINE TimeVal& operator=(const TimeVal &inRhs)
	{
		time_ = inRhs.time_;
		return *this;
	};

	// Use only for relative value, absolute values suffer a lot from precision loss.
	X_INLINE float GetSeconds() const
	{
		return time_ * (1.f / PRECISION);
	}

	// Get relative time difference in seconds - call on the endTime object:  endTime.GetDifferenceInSeconds( startTime );
	X_INLINE float GetDifferenceInSeconds(const TimeVal& startTime) const
	{
		return (time_ - startTime.time_) * (1.f / PRECISION);
	}

	X_INLINE void SetSeconds(const float infSec)
	{
		time_ = (TimeType)(infSec*PRECISION);
	}

	//
	X_INLINE void SetSeconds(const double infSec)
	{
		time_ = (TimeType)(infSec*PRECISION);
	}

	//
	X_INLINE void SetSeconds(const TimeType indwSec)
	{
		time_ = indwSec*PRECISION;
	}

	//
	X_INLINE void SetMilliSeconds(const TimeType indwMilliSec)
	{
		time_ = indwMilliSec*(PRECISION / 1000);
	}

	X_INLINE void SetNanoSeconds(const TimeType indwNanoSec)
	{
		time_ = indwNanoSec / (1000000 / PRECISION);
	}

	// Use only for relative value, absolute values suffer a lot from precision loss.
	X_INLINE float GetMilliSeconds() const
	{
		return time_ * (1000.f / PRECISION);
	}

	X_INLINE TimeType GetMilliSecondsAsInt64() const
	{
		return time_ * 1000 / PRECISION;
	}

	X_INLINE TimeType GetValue() const
	{
		return time_;
	}

	X_INLINE void SetValue(TimeType val)
	{
		time_ = val;
	}


	// Minus.
	X_INLINE TimeVal operator-(const TimeVal &inRhs) const { TimeVal ret;	ret.time_ = time_ - inRhs.time_; return ret; };
	// Plus.
	X_INLINE TimeVal operator+(const TimeVal &inRhs) const { TimeVal ret;	ret.time_ = time_ + inRhs.time_; return ret; };
	// Unary minus.
	X_INLINE TimeVal operator-() const { TimeVal ret; ret.time_ = -time_; return ret; };
	X_INLINE TimeVal& operator+=(const TimeVal& inRhs) { time_ += inRhs.time_; return *this; }
	X_INLINE TimeVal& operator-=(const TimeVal& inRhs) { time_ -= inRhs.time_; return *this; }
	X_INLINE TimeVal& operator/=(int inRhs) { time_ /= inRhs; return *this; }

	// comparison -----------------------
	X_INLINE bool operator<(const TimeVal &inRhs) const { return time_ < inRhs.time_; };
	X_INLINE bool operator>(const TimeVal &inRhs) const { return time_ > inRhs.time_; };
	X_INLINE bool operator>=(const TimeVal &inRhs) const { return time_ >= inRhs.time_; };
	X_INLINE bool operator<=(const TimeVal &inRhs) const { return time_ <= inRhs.time_; };
	X_INLINE bool operator==(const TimeVal &inRhs) const { return time_ == inRhs.time_; };
	X_INLINE bool operator!=(const TimeVal &inRhs) const { return time_ != inRhs.time_; };

private:
	TimeType time_;

	static const TimeType		PRECISION = 3134375;			// one second
};


X_NAMESPACE_END

#endif // _X_TIME_VAL_H_