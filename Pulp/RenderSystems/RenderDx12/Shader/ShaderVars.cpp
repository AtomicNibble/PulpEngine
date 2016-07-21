#include "stdafx.h"
#include "ShaderVars.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(render)

namespace shader
{


	ShaderVars::ShaderVars()
	{
		// defaults
		writeMergedSource_ = 1;
		asyncShaderCompile_ = 1;
	}

	void ShaderVars::RegisterVars(void)
	{
		ADD_CVAR_REF("shader_writeMergedSource", writeMergedSource_, writeMergedSource_, 0, 1,
			core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
			"Writes the merged shader source to file before it's compiled");
		ADD_CVAR_REF("shader_asyncCompile", asyncShaderCompile_, asyncShaderCompile_, 0, 1,
			core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
			"Performs hardware shader compiling async");
	}

} // namespace shader

X_NAMESPACE_END