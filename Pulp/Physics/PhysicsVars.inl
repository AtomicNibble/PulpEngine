#pragma once


X_NAMESPACE_BEGIN(physics)

X_INLINE StepperType::Enum PhysXVars::GetStepperType(void) const
{
	return stepperType_;
}

X_INLINE int32_t PhysXVars::DebugDrawEnabled(void) const
{
	return debugDraw_;
}

X_INLINE float32_t PhysXVars::DebugDrawScale(void) const
{
	return debugDrawScale_;
}


X_NAMESPACE_END