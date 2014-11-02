

template< class type >
X_INLINE XInterpolate<type>::XInterpolate()
{
	startTime = duration = 0;

	core::zero_object(startValue);
	core::zero_object(endValue);
}


template< class type >
X_INLINE void XInterpolate<type>::Init(const int startTime,
	const int duration, const type& startValue, const type& endValue)
{
	this->startTime = startTime;
	this->duration = duration;
	this->startValue = startValue;
	this->endValue = endValue;
}


template< class type >
X_INLINE type XInterpolate<type>::GetCurrentValue(int time) const
{
	if (time <= startTime) {
		return startValue;
	}
	else if (time >= startTime + duration) {
		return endValue;
	}
	else {
		const float deltaTime = time - startTime;
		const float f = deltaTime / (float)duration;
		const type range = (endValue - startValue);
		return startValue + (range * f);
	}
}




template< class type >
X_INLINE XInterpolateAccelDecelLinear<type>::XInterpolateAccelDecelLinear()
{
	startTime = accelTime = linearTime = decelTime = 0;
	core::zero_object(startValue);
	endValue = startValue;
}

template< class type >
X_INLINE void XInterpolateAccelDecelLinear<type>::Init(const int startTime,
	const int accelTime, const int decelTime,
	const int duration, const type &startValue, const type& endValue)
{
	this->startTime = startTime;
	this->accelTime = accelTime;
	this->decelTime = decelTime;
	this->startValue = startValue;
	this->endValue = endValue;

	if (duration <= 0) {
		return;
	}

	if (this->accelTime + this->decelTime > duration) {
		this->accelTime = this->accelTime * duration / (this->accelTime + this->decelTime);
		this->decelTime = duration - this->accelTime;
	}
	this->linearTime = duration - this->accelTime - this->decelTime;
	const type speed = (endValue - startValue) * (1000.0f / ((float) this->linearTime + (this->accelTime + this->decelTime) * 0.5f));

	if (this->accelTime) {
		extrapolate.Init(startTime, this->accelTime, startValue,
			(startValue - startValue), speed, extrapolationType::ACCELLINEAR);
	}
	else if (this->linearTime) {
		extrapolate.Init(startTime, this->linearTime, startValue, 
			(startValue - startValue), speed, extrapolationType::LINEAR);
	}
	else 
	{
		extrapolate.Init(startTime, this->decelTime, startValue, 
			(startValue - startValue), speed, extrapolationType::DECELLINEAR);
	}
}


template< class type >
X_INLINE void XInterpolateAccelDecelLinear<type>::Invalidate()
{
	extrapolate.Init(0, 0, extrapolate.GetStartValue(), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_NONE);
}


template< class type >
X_INLINE void XInterpolateAccelDecelLinear<type>::SetPhase(int time) const
{
	const float deltaTime = static_cast<float>(time - startTime);

	if (deltaTime < accelTime) 
	{
		if (extrapolate.GetExtrapolationType() != extrapolationType::ACCELLINEAR)
		{
			extrapolate.Init(startTime, accelTime, startValue, 
				extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), extrapolationType::ACCELLINEAR);
		}
	}
	else if (deltaTime < accelTime + linearTime) 
	{
		if (extrapolate.GetExtrapolationType() != extrapolationType::LINEAR)
		{
			extrapolate.Init(startTime + accelTime, linearTime, 
				startValue + extrapolate.GetSpeed() * (accelTime * 0.001f * 0.5f),
				extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), extrapolationType::LINEAR);
		}
	}
	else
	{
		if (extrapolate.GetExtrapolationType() != extrapolationType::DECELLINEAR)
		{
			extrapolate.Init(startTime + accelTime + linearTime, decelTime, 
				endValue - (extrapolate.GetSpeed() * (decelTime * 0.001f * 0.5f)),
				extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), extrapolationType::DECELLINEAR);
		}
	}
}

template< class type >
X_INLINE type XInterpolateAccelDecelLinear<type>::GetCurrentValue(int time) const
{
	SetPhase(time);
	return extrapolate.GetCurrentValue(time);
}

template< class type >
X_INLINE type XInterpolateAccelDecelLinear<type>::GetCurrentSpeed(int time) const
{
	SetPhase(time);
	return extrapolate.GetCurrentSpeed(time);
}




template< class type >
X_INLINE XInterpolateAccelDecelSine<type>::XInterpolateAccelDecelSine()
{
	startTime = accelTime = linearTime = decelTime = 0;

	core::zero_object(startValue);
	core::zero_object(endValue);
}


template< class type >
X_INLINE void XInterpolateAccelDecelSine<type>::Init(const int startTime,
	const int accelTime, const int decelTime,
	const int duration, const type &startValue, const type &endValue)
{
	this->startTime = startTime;
	this->accelTime = accelTime;
	this->decelTime = decelTime;
	this->startValue = startValue;
	this->endValue = endValue;

	if (duration <= 0) {
		return;
	}

	if (this->accelTime + this->decelTime > duration) {
		this->accelTime = this->accelTime * duration / (this->accelTime + this->decelTime);
		this->decelTime = duration - this->accelTime;
	}
	this->linearTime = duration - this->accelTime - this->decelTime;
	const type speed = (endValue - startValue) * (1000.0f / ((float) this->linearTime + (this->accelTime + this->decelTime) * idMath::SQRT_1OVER2));

	if (this->accelTime) {
		extrapolate.Init(startTime, this->accelTime, startValue, (startValue - startValue), speed, EXTRAPOLATION_ACCELSINE); //-V501
	}
	else if (this->linearTime) {
		extrapolate.Init(startTime, this->linearTime, startValue, (startValue - startValue), speed, EXTRAPOLATION_LINEAR); //-V501
	}
	else {
		extrapolate.Init(startTime, this->decelTime, startValue, (startValue - startValue), speed, EXTRAPOLATION_DECELSINE); //-V501
	}
}


template< class type >
X_INLINE void XInterpolateAccelDecelSine<type>::Invalidate()
{
	extrapolate.Init(0, 0, extrapolate.GetStartValue(), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_NONE);
}

template< class type >
X_INLINE void XInterpolateAccelDecelSine<type>::SetPhase(int time) const
{
	const float deltaTime = time - startTime;
	if (deltaTime < accelTime) {
		if (extrapolate.GetExtrapolationType() != EXTRAPOLATION_ACCELSINE) {
			extrapolate.Init(startTime, accelTime, startValue, extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_ACCELSINE);
		}
	}
	else if (deltaTime < accelTime + linearTime) {
		if (extrapolate.GetExtrapolationType() != EXTRAPOLATION_LINEAR) {
			extrapolate.Init(startTime + accelTime, linearTime, startValue + extrapolate.GetSpeed() * (accelTime * 0.001f * idMath::SQRT_1OVER2), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_LINEAR);
		}
	}
	else {
		if (extrapolate.GetExtrapolationType() != EXTRAPOLATION_DECELSINE) {
			extrapolate.Init(startTime + accelTime + linearTime, decelTime, endValue - (extrapolate.GetSpeed() * (decelTime * 0.001f * idMath::SQRT_1OVER2)), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_DECELSINE);
		}
	}
}


template< class type >
X_INLINE type XInterpolateAccelDecelSine<type>::GetCurrentValue(int time) const
{
	SetPhase(time);
	return extrapolate.GetCurrentValue(time);
}


template< class type >
X_INLINE type XInterpolateAccelDecelSine<type>::GetCurrentSpeed(int time) const
{
	SetPhase(time);
	return extrapolate.GetCurrentSpeed(time);
}