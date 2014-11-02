#pragma once


#ifndef X_MATH_INTERPOLATE_H_
#define X_MATH_INTERPOLATE_H_


template< class type >
class XInterpolate 
{
public:
	XInterpolate();

	void Init(const int startTime, const int duration,
		const type& startValue, const type& endValue);
	void SetStartTime(int time) { this->startTime = time; }
	void SetDuration(int duration) { this->duration = duration; }
	void SetStartValue(const type &startValue) { this->startValue = startValue; }
	void SetEndValue(const type &endValue) { this->endValue = endValue; }

	type GetCurrentValue(int time) const;
	bool IsDone(int time) const { return (time >= startTime + duration); }

	int	GetStartTime() const { return startTime; }
	int	GetEndTime() const { return startTime + duration; }
	int	GetDuration() const { return duration; }
	const type&	GetStartValue() const { return startValue; }
	const type&	GetEndValue() const { return endValue; }

private:
	int		startTime;
	int		duration;
	type	startValue;
	type	endValue;
};

template< class type >
class XInterpolateAccelDecelLinear
{
public:
	XInterpolateAccelDecelLinear();

	void Init(const int startTime, const int accelTime, const int decelTime, 
		const int duration, const type& startValue, const type& endValue);
	void SetStartTime(int time) { startTime = time; Invalidate(); }
	void SetStartValue(const type &startValue) { this->startValue = startValue; Invalidate(); }
	void SetEndValue(const type &endValue) { this->endValue = endValue; Invalidate(); }

	type GetCurrentValue(int time) const;
	type GetCurrentSpeed(int time) const;
	bool IsDone(int time) const { return (time >= startTime + accelTime + linearTime + decelTime); }

	int	GetStartTime() const { return startTime; }
	int	GetEndTime() const { return startTime + accelTime + linearTime + decelTime; }
	int	GetDuration() const { return accelTime + linearTime + decelTime; }
	int	GetAcceleration() const { return accelTime; }
	int	GetDeceleration() const { return decelTime; }
	const type& GetStartValue() const { return startValue; }
	const type& GetEndValue() const { return endValue; }

private:
	int		startTime;
	int		accelTime;
	int		linearTime;
	int		decelTime;
	type	startValue;
	type	endValue;
	mutable XExtrapolate<type> extrapolate;

	void Invalidate();
	void SetPhase(int time) const;
};



template< class type >
class XInterpolateAccelDecelSine
{
public:
	XInterpolateAccelDecelSine();

	void Init(const int startTime, const int accelTime, 
		const int decelTime, const int duration,
		const type& startValue, const type& endValue);
	void SetStartTime(int time) { startTime = time; Invalidate(); }
	void SetStartValue(const type &startValue) { this->startValue = startValue; Invalidate(); }
	void SetEndValue(const type &endValue) { this->endValue = endValue; Invalidate(); }

	type GetCurrentValue(int time) const;
	type GetCurrentSpeed(int time) const;
	bool IsDone(int time) const { return (time >= startTime + accelTime + linearTime + decelTime); }

	int GetStartTime() const { return startTime; }
	int GetEndTime() const { return startTime + accelTime + linearTime + decelTime; }
	int GetDuration() const { return accelTime + linearTime + decelTime; }
	int GetAcceleration() const { return accelTime; }
	int GetDeceleration() const { return decelTime; }
	const type&	GetStartValue() const { return startValue; }
	const type&	GetEndValue() const { return endValue; }

private:
	int startTime;
	int accelTime;
	int linearTime;
	int decelTime;
	type startValue;
	type endValue;
	mutable XExtrapolate<type> extrapolate;

	void Invalidate();
	void SetPhase(int time) const;
};


#include "XInterpolate.inl"

#endif // !X_MATH_INTERPOLATE_H_ 
