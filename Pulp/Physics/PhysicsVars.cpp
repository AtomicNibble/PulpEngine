#include "stdafx.h"
#include "PhysicsVars.h"

#include <IConsole.h>


X_NAMESPACE_BEGIN(physics)



PhysXVars::PhysXVars() :
	pVarScratchBufSize_(nullptr),
	pVarStepperType_(nullptr)
{

	scratchBufferDefaultSize_ = 16; // 16 KiB

	stepperType_ = StepperType::FIXED_STEPPER;
}


void PhysXVars::RegisterVars(void)
{
	pVarScratchBufSize_ = ADD_CVAR_INT("phys_scratch_buf_size", scratchBufferDefaultSize_, 0, std::numeric_limits<int32_t>::max(),
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED | core::VarFlag::RESTART_REQUIRED, 
		"Size of the scratch buffer in kib, must be a multiple of 16.");

	pVarStepperType_ = ADD_CVAR_INT("phys_stepper_style", stepperType_, 0, StepperType::ENUM_COUNT - 1,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Stepper style for physics update. 0=debug, 1=fixed, 2=inverted-fixed, 3=variable.");

	core::ConsoleVarFunc del;
	del.Bind<PhysXVars, &PhysXVars::Var_OnStepperStyleChange>(this);
	pVarStepperType_->SetOnChangeCallback(del);


	ADD_CVAR_REF("physx_draw_debug_shapes", debugDraw_, 0, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED, 
		"Enable drawing of physics debug shapes");
	ADD_CVAR_REF("physx_draw_debug_scale", debugDrawScale_, 1.0, 0.f, 10.f, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Debug draw scale");
}


uint32_t PhysXVars::ScratchBufferSize(void) const
{
	return safe_static_cast<uint32_t, int32_t>(pVarScratchBufSize_->GetInteger() << 10);
}

void PhysXVars::Var_OnStepperStyleChange(core::ICVar* pVar)
{
	int32_t val = pVar->GetInteger();

	if (val < 0 && val >= StepperType::ENUM_COUNT) {
		X_ASSERT_UNREACHABLE();
		return;
	}

	stepperType_ = static_cast<StepperType::Enum>(val);
}


X_NAMESPACE_END