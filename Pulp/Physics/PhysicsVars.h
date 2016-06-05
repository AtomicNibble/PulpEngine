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

private:
	void Var_OnStepperStyleChange(core::ICVar* pVar);

private:
	core::ICVar* pVarScratchBufSize_;
	core::ICVar* pVarStepperType_;

	int32_t scratchBufferDefaultSize_;
	StepperType::Enum stepperType_;
};


X_NAMESPACE_END

#include "PhysicsVars.inl"