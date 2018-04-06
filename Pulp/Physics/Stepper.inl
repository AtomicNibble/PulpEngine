#pragma once

X_NAMESPACE_BEGIN(physics)

X_INLINE void Stepper::renderDone(void)
{
}

X_INLINE void Stepper::shutdown(void)
{
}

X_INLINE core::TimeVal Stepper::getSimulationTime(void) const
{
    return simulationTime_;
}

X_INLINE IStepperHandler& Stepper::getHandler(void)
{
    return *pHandler_;
}

X_INLINE const IStepperHandler& Stepper::getHandler(void) const
{
    return *pHandler_;
}

X_INLINE void Stepper::setHandler(IStepperHandler* pHanlder)
{
    pHandler_ = pHanlder;
}

X_INLINE void Stepper::setSubStepper(const float32_t stepSize, const uint32_t maxSteps)
{
    X_UNUSED(stepSize);
    X_UNUSED(maxSteps);
}

// --------------------------------------------------

X_INLINE void StepperTask::setStepper(MultiThreadStepper* stepper)
{
    pStepper_ = stepper;
}

X_INLINE MultiThreadStepper* StepperTask::getStepper(void)
{
    return pStepper_;
}

X_INLINE const MultiThreadStepper* StepperTask::getStepper(void) const
{
    return pStepper_;
}

X_INLINE const char* StepperTask::getName(void) const
{
    return "Stepper Task";
}

// --------------------------------------------------

X_INLINE void MultiThreadStepper::postRender(const float32_t stepSize)
{
    X_UNUSED(stepSize);
}

X_INLINE void MultiThreadStepper::wait(physx::PxScene* scene)
{
    // if mNbSubSteps is 0 then the sync will never
    // be set so waiting would cause a deadlock
    if (nbSubSteps_) {
        sync_.wait();
    }
}

X_INLINE void MultiThreadStepper::shutdown(void)
{
}

X_INLINE float32_t MultiThreadStepper::getSubStepSize(void) const
{
    return subStepSize_;
}

// --------------------------------------------------

X_INLINE void DebugStepper::postRender(const float32_t stepSize)
{
    X_UNUSED(stepSize);
}

// --------------------------------------------------

X_INLINE void FixedStepper::reset(void)
{
    accumulator_ = 0.0f;
}

X_INLINE void FixedStepper::postRender(const float32_t stepSize)
{
    X_UNUSED(stepSize);
}

// --------------------------------------------------

X_INLINE void VariableStepper::reset(void)
{
    accumulator_ = 0.0f;
}

X_NAMESPACE_END