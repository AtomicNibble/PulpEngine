

X_NAMESPACE_BEGIN(physics)

X_INLINE bool XPhysics::IsPaused(void) const
{
    return pause_;
}

X_INLINE void XPhysics::togglePause(void)
{
    pause_ = !pause_;
}

X_INLINE void XPhysics::setSubStepper(const float32_t stepSize, const uint32_t maxSteps)
{
    getStepper()->setSubStepper(stepSize, maxSteps);
}

X_NAMESPACE_END