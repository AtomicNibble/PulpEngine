#include "stdafx.h"
#include "PhysicsVars.h"

#include <IConsole.h>


X_NAMESPACE_BEGIN(physics)



PhysXVars::PhysXVars() :
	pScene_(nullptr),
	pVarScratchBufSize_(nullptr),
	pVarStepperType_(nullptr)
{
	scratchBufferDefaultSize_ = 16; // 16 KiB
	stepperType_ = StepperType::FIXED_STEPPER;

	debugDraw_ = 0;

	core::zero_object(scaleVarNames_);
	core::zero_object(scaleVars_);

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

	// toggle drawing on off. seperate to the scales.
	ADD_CVAR_REF("phys_draw_debug", debugDraw_, 0, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED, 
		"Enable drawing of physics debug shapes");

	// all the scales yo.
	// we need to register call backs for all these so we can update the scene.
	core::ConsoleVarFunc scaleChangedDel;
	scaleChangedDel.Bind<PhysXVars, &PhysXVars::Var_OnScaleChanged>(this);

	// we have to define the names manually since physx declares anormal enum so we don't have tostring like engine enums.
	// plus defining the full name as a const char means we don't have to allocate memory for each one to store the full name.
	scaleVarNames_[physx::PxVisualizationParameter::eSCALE]   					= "phys_draw_debug_scale";
	scaleVarNames_[physx::PxVisualizationParameter::eWORLD_AXES]   				= "phys_draw_debug_scale_world_axes";
	scaleVarNames_[physx::PxVisualizationParameter::eBODY_AXES]   				= "phys_draw_debug_scale_body_axes";
	scaleVarNames_[physx::PxVisualizationParameter::eBODY_MASS_AXES]   			= "phys_draw_debug_scale_body_mass_axes";
	scaleVarNames_[physx::PxVisualizationParameter::eBODY_LIN_VELOCITY]   		= "phys_draw_debug_scale_body_lin_verlocity";
	scaleVarNames_[physx::PxVisualizationParameter::eBODY_ANG_VELOCITY]   		= "phys_draw_debug_scale_body_ang_velocity";
	scaleVarNames_[physx::PxVisualizationParameter::eBODY_JOINT_GROUPS]   		= "phys_draw_debug_scale_body_joint_groups";
	scaleVarNames_[physx::PxVisualizationParameter::eCONTACT_POINT]   			= "phys_draw_debug_scale_contat_point";
	scaleVarNames_[physx::PxVisualizationParameter::eCONTACT_NORMAL]   			= "phys_draw_debug_scale_contat_normal";
	scaleVarNames_[physx::PxVisualizationParameter::eCONTACT_ERROR]   			= "phys_draw_debug_scale_contat_error";
	scaleVarNames_[physx::PxVisualizationParameter::eCONTACT_FORCE]   			= "phys_draw_debug_scale_contat_force";
	scaleVarNames_[physx::PxVisualizationParameter::eACTOR_AXES]   				= "phys_draw_debug_scale_actor_axes";
	scaleVarNames_[physx::PxVisualizationParameter::eCOLLISION_AABBS]   		= "phys_draw_debug_scale_col_aabbs";
	scaleVarNames_[physx::PxVisualizationParameter::eCOLLISION_SHAPES]   		= "phys_draw_debug_scale_col_shapes";
	scaleVarNames_[physx::PxVisualizationParameter::eCOLLISION_AXES]   			= "phys_draw_debug_scale_col_axes";
	scaleVarNames_[physx::PxVisualizationParameter::eCOLLISION_COMPOUNDS]   	= "phys_draw_debug_scale_col_compounds";
	scaleVarNames_[physx::PxVisualizationParameter::eCOLLISION_FNORMALS]   		= "phys_draw_debug_scale_col_fnormals";
	scaleVarNames_[physx::PxVisualizationParameter::eCOLLISION_EDGES]   		= "phys_draw_debug_scale_col_edges";
	scaleVarNames_[physx::PxVisualizationParameter::eCOLLISION_STATIC]   		= "phys_draw_debug_scale_col_static";
	scaleVarNames_[physx::PxVisualizationParameter::eCOLLISION_DYNAMIC]   		= "phys_draw_debug_scale_col_dynamic";
	scaleVarNames_[physx::PxVisualizationParameter::eCOLLISION_PAIRS]   		= "phys_draw_debug_scale_col_pairs";
	scaleVarNames_[physx::PxVisualizationParameter::eJOINT_LOCAL_FRAMES]   		= "phys_draw_debug_scale_joint_local_frames";
	scaleVarNames_[physx::PxVisualizationParameter::eJOINT_LIMITS]   			= "phys_draw_debug_scale_joint_limits";
	// skipped particle ones for now
	// ..
	scaleVarNames_[physx::PxVisualizationParameter::eCULL_BOX]   				= "phys_draw_debug_scale_cull_box";
	// skipped cloth ones for now
	// ..
	scaleVarNames_[physx::PxVisualizationParameter::eMBP_REGIONS]   			= "phys_draw_debug_scale_mpb_regions";
	
	// lets get fancy and auto gen the vars.
	for (uint32_t i = 0; i < physx::PxVisualizationParameter::eNUM_VALUES; i++)
	{
		// if name is not define we just don't expose it.
		if (!scaleVarNames_[i]) {
			continue;
		}

		scaleVars_[i] = ADD_CVAR_FLOAT(scaleVarNames_[i], 0.f, 0.f, 10.f, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED, "Debug draw scale");
		scaleVars_[i]->SetOnChangeCallback(scaleChangedDel);
	}

}

void PhysXVars::SetScene(physx::PxScene* pScene)
{
	pScene_ = pScene;
}

uint32_t PhysXVars::ScratchBufferSize(void) const
{
	return safe_static_cast<uint32_t, int32_t>(pVarScratchBufSize_->GetInteger() << 10);
}


void PhysXVars::Var_OnScaleChanged(core::ICVar* pVar)
{
	if (!pScene_) {
		X_ERROR("Phys", "Can't update vis scale without a valid scene");
		return;
	}

	// we find the enum from the index of the var.
	for (uint32_t i = 0; i < physx::PxVisualizationParameter::eNUM_VALUES; i++)
	{
		if (scaleVars_[i] == pVar)
		{
			const float val = pVar->GetFloat();

			physx::PxSceneWriteLock scopedLock(*pScene_);
			pScene_->setVisualizationParameter(static_cast<physx::PxVisualizationParameter::Enum>(i), val);
			return;
		}
	}

	X_ERROR("Phys", "Failed to find scale var in lookup: \"%s\"", pVar->GetName());
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