
X_NAMESPACE_BEGIN(anim)



X_INLINE float AnimBlend::getStartWeight(void) const
{
	return blendStartVal_;
}

X_INLINE float AnimBlend::getFinalWeight(void) const
{
	return blendEndVal_;
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
