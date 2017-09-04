
X_NAMESPACE_BEGIN(anim)


X_INLINE const Anim* AnimBlend::getAnim(void) const
{
	return pAnim_;
}

X_INLINE float AnimBlend::getStartWeight(void) const
{
	return blendStartVal_;
}

X_INLINE float AnimBlend::getFinalWeight(void) const
{
	return blendEndVal_;
}

X_INLINE float AnimBlend::getRate(void) const
{
	return rate_;
}

X_INLINE core::TimeVal AnimBlend::getBlendStart(void) const
{
	return blendStart_;
}

X_INLINE core::TimeVal AnimBlend::getBlendDuration(void) const
{
	return blendDuration_;
}

X_INLINE core::TimeVal AnimBlend::getStartTime(void) const
{
	return startTime_;
}


// ---------------------------------------


X_INLINE size_t Animator::numAnims(void) const
{
	X_ASSERT_NOT_IMPLEMENTED();
	return 0_sz;
}

X_INLINE const Animator::Mat44Arr& Animator::getBoneMatrices(void) const
{
	return boneMat_;
}


X_NAMESPACE_END
