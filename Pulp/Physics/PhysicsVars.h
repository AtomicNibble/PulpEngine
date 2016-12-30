#pragma once

#include "Stepper.h"

X_NAMESPACE_DECLARE(core,
	struct ICVar;
)

X_NAMESPACE_BEGIN(physics)


class PhysXVars
{
public:
	PhysXVars();
	~PhysXVars() = default;

	void RegisterVars(void);

	uint32_t ScratchBufferSize(void) const;
	StepperType::Enum GetStepperType(void) const;

	int32_t DebugDrawEnabled(void) const;
	float32_t DebugDrawScale(void) const;

private:
	void Var_OnStepperStyleChange(core::ICVar* pVar);

private:
	core::ICVar* pVarScratchBufSize_;
	core::ICVar* pVarStepperType_;

	int32_t scratchBufferDefaultSize_;
	StepperType::Enum stepperType_;

	int32_t debugDraw_;
	float debugDrawScale_;
};


X_NAMESPACE_END

#include "PhysicsVars.inl"