#pragma once

#ifndef X_MATH_EXTRAPOLATE_H_
#define X_MATH_EXTRAPOLATE_H_

#include <Util\EnumMacros.h>


X_DECLARE_ENUM(extrapolationType)(
	NONE,			// no extrapolation, covered distance = duration * 0.001 * ( baseSpeed )
	LINEAR,			// linear extrapolation, covered distance = duration * 0.001 * ( baseSpeed + speed )
	ACCELLINEAR,	// linear acceleration, covered distance = duration * 0.001 * ( baseSpeed + 0.5 * speed )
	DECELLINEAR,	// linear deceleration, covered distance = duration * 0.001 * ( baseSpeed + 0.5 * speed )
	ACCELSINE,		// sinusoidal acceleration, covered distance = duration * 0.001 * ( baseSpeed + sqrt( 0.5 ) * speed )
	DECELSINE		// sinusoidal deceleration, covered distance = duration * 0.001 * ( baseSpeed + sqrt( 0.5 ) * speed )
);

template< class type >
class XExtrapolate 
{
public:
	XExtrapolate();

	void Init(const int startTime, const int duration, const type& startValue,
		const type& baseSpeed, const type& speed, const extrapolationType::Enum extrapolationType);
	type GetCurrentValue(int time) const;
	type GetCurrentSpeed(int time) const;
	bool IsDone(int time) const { return (time >= startTime + duration); }
	void SetStartTime(int time) { startTime = time; }
	int	GetStartTime() const { return startTime; }
	int	GetEndTime() const { return (duration > 0) ? startTime + duration : 0; }
	int	GetDuration() const { return duration; }
	void SetStartValue(const type &value) { startValue = value; }
	const type&	GetStartValue() const { return startValue; }
	const type&	GetBaseSpeed() const { return baseSpeed; }
	const type&	GetSpeed() const { return speed; }
	extrapolationType::Enum	GetExtrapolationType() const { return extrapolationType; }

private:
	extrapolationType::Enum	extrapolationType;
	int	startTime;
	int	duration;
	type startValue;
	type baseSpeed;
	type speed;
};


template< class type >
X_INLINE XExtrapolate<type>::XExtrapolate()
{
	extrapolationType = extrapolationType::NONE;
	startTime = duration = 0;

	core::zero_object(startValue);
	core::zero_object(baseSpeed);
	core::zero_object(speed);
}


template< class type >
X_INLINE void XExtrapolate<type>::Init(const int startTime, const int duration,
	const type& startValue, const type& baseSpeed, 
	const type& speed, const extrapolationType::Enum extrapolationType)
{
	this->extrapolationType = extrapolationType;
	this->startTime = startTime;
	this->duration = duration;
	this->startValue = startValue;
	this->baseSpeed = baseSpeed;
	this->speed = speed;
}


template< class type >
X_INLINE type XExtrapolate<type>::GetCurrentValue(int time) const
{
	if (time < startTime) {
		return startValue;
	}
	
	if ((time > startTime + duration)) {
		time = startTime + duration;
	}
	
	switch (extrapolationType)
	{
		case extrapolationType::NONE:
		{
			const float deltaTime = (time - startTime) * 0.001f;
			return startValue + deltaTime * baseSpeed;
		}
		case extrapolationType::LINEAR:
		{
			const float deltaTime = (time - startTime) * 0.001f;
			return startValue + deltaTime * (baseSpeed + speed);
		}
		case extrapolationType::ACCELLINEAR:
		{
			if (duration == 0) {
				return startValue;
			}
			else {
				const float deltaTime = (time - startTime) / (float)duration;
				const float s = (0.5f * deltaTime * deltaTime) * 
					((float)duration * 0.001f);
				return startValue + deltaTime * baseSpeed + s * speed;
			}
		}
		case extrapolationType::DECELLINEAR:
		{
			if (duration == 0) {
				return startValue;
			}
			else {
				const float deltaTime = (time - startTime) / (float)duration;
				const float s = (deltaTime - (0.5f * deltaTime * deltaTime)) * 
					((float)duration * 0.001f);
				return startValue + deltaTime * baseSpeed + s * speed;
			}
		}
		case extrapolationType::ACCELSINE:
			if (duration == 0) {
				return startValue;
			}
			else {
				const float deltaTime = (time - startTime) / (float)duration;
				const float s = (1.0f - math<float>::cos(deltaTime * PIHalff))
					* (float)duration * 0.001f * Sqrt_1OVER2;
				return startValue + deltaTime * baseSpeed + s * speed;
			}

		case extrapolationType::DECELSINE:
			if (duration == 0) {
				return startValue;
			}
			else {
				const float deltaTime = (time - startTime) / (float)duration;
				const float s = math<float>::sin(deltaTime * PIHalff) *
					(float)duration * 0.001f * Sqrt_1OVER2;
				return startValue + deltaTime * baseSpeed + s * speed;
			}

	}

	return startValue;
}


template< class type >
X_INLINE type XExtrapolate<type>::GetCurrentSpeed(int time) const
{
	if (time < startTime || duration == 0) {
		return (startValue - startValue); 
	}

	if ((time > startTime + duration)) {
		return (startValue - startValue); 
	}

	switch (extrapolationType)
	{
		case extrapolationType::NONE:
			return baseSpeed;

		case extrapolationType::LINEAR:
			return baseSpeed + speed;

		case extrapolationType::ACCELLINEAR:
			const float deltaTime = (time - startTime) / (float)duration;
			const float s = deltaTime;
			return baseSpeed + s * speed;

		case extrapolationType::DECELLINEAR:
			const float deltaTime = (time - startTime) / (float)duration;
			const float s = 1.0f - deltaTime;
			return baseSpeed + s * speed;

		case extrapolationType::ACCELSINE:
			const float deltaTime = (time - startTime) / (float)duration;
			const float s = idMath::Sin(deltaTime * idMath::HALF_PI);
			return baseSpeed + s * speed;

		case extrapolationType::DECELSINE:
			const float deltaTime = (time - startTime) / (float)duration;
			const float s = idMath::Cos(deltaTime * idMath::HALF_PI);
			return baseSpeed + s * speed;

		default:
			return baseSpeed;		
	}
}


#endif // !X_MATH_EXTRAPOLATE_H_