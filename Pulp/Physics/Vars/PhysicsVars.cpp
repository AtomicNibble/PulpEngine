#include "stdafx.h"
#include "PhysicsVars.h"
#include "Util/MathHelpers.h"

#include <IConsole.h>


X_NAMESPACE_BEGIN(physics)



PhysXVars::PhysXVars() :
	pScene_(nullptr),
	pVarDllOverride_(nullptr),
	pVarScratchBufSize_(nullptr),
	pVarStepperType_(nullptr),
	pVarDebugDraw_(nullptr),
	pVarPvdIp_(nullptr)
{
	scratchBufferDefaultSize_ = 16; // 16 KiB
	trackAllocations_ = 0;
	stepperType_ = StepperType::FIXED_STEPPER;

	debugDraw_ = 0;
	unifiedHeightFields_ = 0;

	core::zero_object(scaleVarNames_);
	core::zero_object(scaleVars_);

}


void PhysXVars::RegisterVars(void)
{
	core::ConsoleVarFunc del;

	pVarDllOverride_ = ADD_CVAR_STRING("phys_dll_override", "none", core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED | core::VarFlag::RESTART_REQUIRED,
		"Override which physx dll is loaded. Valid Values: <none>, <debug>, <checked>, <profile>, <release>");

	pVarScratchBufSize_ = ADD_CVAR_INT("phys_scratch_buf_size", scratchBufferDefaultSize_, 0, std::numeric_limits<int32_t>::max(),
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED | core::VarFlag::RESTART_REQUIRED, 
		"Size of the scratch buffer in KiB, must be a multiple of 16.");

	ADD_CVAR_REF("phys_alloc_track", trackAllocations_, trackAllocations_, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED | core::VarFlag::RESTART_REQUIRED,
		"Enable physics allocation tracking");


	del.Bind<PhysXVars, &PhysXVars::Var_OnStepperStyleChange>(this);

	pVarStepperType_ = ADD_CVAR_INT("phys_stepper_style", stepperType_, 0, StepperType::ENUM_COUNT - 1,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Stepper style for physics update. 0=debug, 1=fixed, 2=inverted-fixed, 3=variable.")->SetOnChangeCallback(del);

	ADD_CVAR_REF("phys_pvd_enable", pvdEnable_, 0, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED | core::VarFlag::RESTART_REQUIRED,
		"Enable PVD connections. Must be enabled before you can even attempt to connect");

	ADD_CVAR_REF("phys_pvd_port", pvdPort_, 5425, 0, std::numeric_limits<uint16_t>::max(), core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"PVD connection port");

	ADD_CVAR_REF("phys_pvd_timeout", pvdTineoutMS_, 10, 0, std::numeric_limits<uint16_t>::max(), core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"PVD connection tomeout in MS");

	ADD_CVAR_REF("phys_pvd_flags", pvdFlags_, core::bitUtil::AlphaBits("dpm"), 0, core::bitUtil::AlphaBits("dpm"),
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED | core::VarFlag::BITFIELD,
		"PVD connection Flags. "
		"d: Debug, "
		"p: Profile, "
		"m: Memory"
	);

	pVarPvdIp_ = ADD_CVAR_STRING("phys_pvd_ip", "127.0.0.1", core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"PVD connection ip");


	del.Bind<PhysXVars, &PhysXVars::Var_OnDebugDrawChange>(this);

	// toggle drawing on off. seperate to the scales.
	pVarDebugDraw_ = ADD_CVAR_REF("phys_draw_debug", debugDraw_, 1, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Enable drawing of physics debug shapes")->SetOnChangeCallback(del);

	ADD_CVAR_REF("phys_unified_height_fields", unifiedHeightFields_, 0, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED | core::VarFlag::RESTART_REQUIRED,
		"Enable unified height fields");

	del.Bind<PhysXVars, &PhysXVars::Var_OnDebugUseCullChange>(this);

	ADD_CVAR_REF("phys_draw_debug_cull", debugDrawUseCullBox_, 1, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Enable culling of physics debug shapes")->SetOnChangeCallback(del);

	// this is read only.
	ADD_CVAR_REF_VEC3("phys_gravity", gravityVec_, Vec3f::zAxis(), core::VarFlag::SYSTEM | core::VarFlag::READONLY,
		"The gavity vector");

	// all the scales yo.
	// we need to register call backs for all these so we can update the scene.
	core::ConsoleVarFunc scaleChangedDel;
	scaleChangedDel.Bind<PhysXVars, &PhysXVars::Var_OnScaleChanged>(this);

	// we have to define the names manually since physx declares a normal enum so we don't have toString() like engine enums.
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
// this enables cull box not visulization of cull box.
//	scaleVarNames_[physx::PxVisualizationParameter::eCULL_BOX]   				= "phys_draw_debug_scale_cull_box";
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

		auto* pVar = ADD_CVAR_FLOAT(scaleVarNames_[i], 0.f, 0.f, 128.f, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED, "Debug draw scale");
		pVar->SetOnChangeCallback(scaleChangedDel);

		scaleVars_[i] = pVar;
	}
}

void PhysXVars::SetScene(physx::PxScene* pScene)
{
	pScene_ = pScene;

	PHYS_SCENE_WRITE_LOCK(pScene_);

	SetGravityVecValue(Vec3FromPx3(pScene_->getGravity()));

	// set current values.
	for (uint32_t i = 0; i < physx::PxVisualizationParameter::eNUM_VALUES; i++)
	{		
		const auto* pVar = scaleVars_[i];
		if (pVar)
		{
			const float val = pVar->GetFloat();

			if (val > 0.f) {
				pScene_->setVisualizationParameter(static_cast<physx::PxVisualizationParameter::Enum>(i), val);
			}
		}
	}
}

void PhysXVars::ClearScene(void)
{
	pScene_ = nullptr;
}

void PhysXVars::SetDebugDrawChangedDel(DebugDrawEnabledDel del)
{
	debugDrawChangedDel_ = del;
}


const char* PhysXVars::getDllOverrideStr(StrBuf& buf) const
{
	X_ASSERT_NOT_NULL(pVarDllOverride_);
	return pVarDllOverride_->GetString(buf);
}

uint32_t PhysXVars::scratchBufferSize(void) const
{
	X_ASSERT_NOT_NULL(pVarScratchBufSize_);
	return safe_static_cast<uint32_t, int32_t>(pVarScratchBufSize_->GetInteger() << 10);
}


const char* PhysXVars::getPVDIp(StrBuf& buf) const
{
	X_ASSERT_NOT_NULL(pVarPvdIp_);
	return pVarPvdIp_->GetString(buf);
}

void PhysXVars::SetDebugDrawEnabled(bool enable)
{
	if (pVarDebugDraw_) {
		pVarDebugDraw_->Set(enable ? 1 : 0);
	}
	else {
		debugDraw_ = enable ? 1 : 0;
	}
}

void PhysXVars::SetGravityVecValue(const Vec3f& gravity)
{
	gravityVec_ = gravity;
}

void PhysXVars::SetAllScalesToValue(float32_t val)
{
	for (uint32_t i = 0; i < physx::PxVisualizationParameter::eNUM_VALUES; i++)
	{
		if (scaleVars_[i])
		{
			// this will result in the Var_OnScaleChanged getting called.
			// so the scene scales will get updated.
			scaleVars_[i]->Set(val);
		}
	}
}

void PhysXVars::Var_OnDebugDrawChange(core::ICVar* pVar)
{
	X_UNUSED(pVar);
	// so this is like my var to turn my drawing on / off.
	// but we should also turn off the physx scale to increase performance.
	// i don't think i'll bother changing the console var value for 'phys_draw_debug_scale'
	// this can just be a slient internal change.

	// if we don't have a scene still update scale var for when we do.
	core::ICVar* pScaleVar = scaleVars_[physx::PxVisualizationParameter::eSCALE];
	if (debugDraw_ && pScaleVar)
	{
		const  float scale = pScaleVar->GetFloat();
		if (scale == 0.f) 
		{
			pScaleVar->Set(1.f);
		}
	}

	if (!pScene_) {
		// we might not have a scene yet, when we do have one current values are transfferd to the scene.
		return;
	}

	PHYS_SCENE_WRITE_LOCK(pScene_);

	if (debugDraw_)
	{
		// check current value.
		if (pScaleVar)
		{
			const float scale = pScaleVar->GetFloat();

			pScene_->setVisualizationParameter(physx::PxVisualizationParameter::eSCALE, scale);
		}
	}
	else
	{
		pScene_->setVisualizationParameter(physx::PxVisualizationParameter::eSCALE, 0);
	}

	if (debugDrawChangedDel_) {
		debugDrawChangedDel_.Invoke(debugDraw_ != 0);
	}
}

void PhysXVars::Var_OnScaleChanged(core::ICVar* pVar)
{
	if (!pScene_) {
		return;
	}

	// we find the enum from the index of the var.
	for (uint32_t i = 0; i < physx::PxVisualizationParameter::eNUM_VALUES; i++)
	{
		if (scaleVars_[i] == pVar)
		{
			const float val = pVar->GetFloat();

			PHYS_SCENE_WRITE_LOCK(pScene_);
			pScene_->setVisualizationParameter(static_cast<physx::PxVisualizationParameter::Enum>(i), val);
			return;
		}
	}

	X_ERROR("Phys", "Failed to find scale var in lookup: \"%s\"", pVar->GetName());
}

void PhysXVars::Var_OnDebugUseCullChange(core::ICVar* pVar)
{
	// o my.
	// you want to cull me baby?
	// well then..

	if (!pScene_) {
		X_ERROR("Phys", "Can't update vis culling without a valid scene");
		return;
	}

	PHYS_SCENE_WRITE_LOCK(pScene_);

	if (pVar->GetInteger() == 0)
	{
		pScene_->setVisualizationParameter(physx::PxVisualizationParameter::eCULL_BOX, 0);
	}
	else
	{
		// when we enable this we need to ensure a valid bounds is set with 'setVisualizationCullingBox'
		// this needs to be done before the next call to simulate.
		pScene_->setVisualizationParameter(physx::PxVisualizationParameter::eCULL_BOX, 1);
	}
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