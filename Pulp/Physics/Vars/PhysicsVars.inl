#pragma once

X_NAMESPACE_BEGIN(physics)

X_INLINE bool PhysXVars::enableAllocTracking(void) const
{
    return trackAllocations_ != 0;
}

X_INLINE StepperType::Enum PhysXVars::getStepperType(void) const
{
    return stepperType_;
}

X_INLINE bool PhysXVars::isPVDEnabled(void) const
{
    return pvdEnable_ != 0;
}

X_INLINE int32_t PhysXVars::getPVDPort(void) const
{
    return pvdPort_;
}

X_INLINE int32_t PhysXVars::getPVDTimeoutMS(void) const
{
    return pvdTineoutMS_;
}

X_INLINE int32_t PhysXVars::getPVDFlags(void) const
{
    return pvdFlags_;
}

X_INLINE int32_t PhysXVars::DebugDrawEnabled(void) const
{
    return debugDraw_;
}

X_INLINE int32_t PhysXVars::DebugDrawCullEnabled(void) const
{
    return debugDrawUseCullBox_;
}

X_INLINE int32_t PhysXVars::UnifiedHeightFieldsEnabled(void) const
{
    return unifiedHeightFields_;
}

X_NAMESPACE_END