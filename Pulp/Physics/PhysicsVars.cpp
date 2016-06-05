#include "stdafx.h"
#include "PhysicsVars.h"

#include <IConsole.h>

#include "XPhysics.h"

X_NAMESPACE_BEGIN(physics)

PhysXVars::PhysXVars() :
	pVarScratchBufSize_(nullptr)
{

	scratchBufferDefaultSize_ = 16; // 16 KiB
}


void PhysXVars::RegisterVars(void)
{
	pVarScratchBufSize_ = ADD_CVAR_INT("phys_scratch_buf_size", scratchBufferDefaultSize_, 0, std::numeric_limits<int32_t>::max(),
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED | core::VarFlag::RESTART_REQUIRED, 
		"Size of the scratch buffer in kib, must be a multiple of 16.");


}


uint32_t PhysXVars::ScratchBufferSize(void) const
{
	return safe_static_cast<uint32_t, int32_t>(pVarScratchBufSize_->GetInteger());
}




X_NAMESPACE_END