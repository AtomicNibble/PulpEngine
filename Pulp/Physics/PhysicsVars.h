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
	void SetScene(physx::PxScene* pScene);

	uint32_t ScratchBufferSize(void) const;
	X_INLINE StepperType::Enum GetStepperType(void) const;

	X_INLINE int32_t DebugDrawEnabled(void) const;
	X_INLINE int32_t DebugDrawCullEnabled(void) const;

private:
	void Var_OnScaleChanged(core::ICVar* pVar);
	void Var_OnDebugUseCullChange(core::ICVar* pVar);
	void Var_OnStepperStyleChange(core::ICVar* pVar);

private:
	physx::PxScene* pScene_;

	core::ICVar* pVarScratchBufSize_;
	core::ICVar* pVarStepperType_;

	int32_t scratchBufferDefaultSize_;
	StepperType::Enum stepperType_;

	int32_t debugDraw_;
	int32_t debugDrawUseCullBox_;

	const char* scaleVarNames_[physx::PxVisualizationParameter::eNUM_VALUES];
	core::ICVar* scaleVars_[physx::PxVisualizationParameter::eNUM_VALUES];
};


X_NAMESPACE_END

#include "PhysicsVars.inl"