#include "stdafx.h"
#include "ShaderVars.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(render)

namespace shader
{


	ShaderVars::ShaderVars()
	{
		// defaults
		useCache_ = 1;
		writeCompiledShaders_ = 1;
		writeMergedSource_ = 1;
		compileDebug_ = 0;
		helpWithWorkOnShaderStall_ = 1;
	}

	void ShaderVars::RegisterVars(void)
	{
		ADD_CVAR_REF("shader_load_from_cache", useCache_, useCache_, 0, 1,
			core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
			"Loads shader from shader bin if present");

		ADD_CVAR_REF("shader_write_compiled_shaders", writeCompiledShaders_, writeCompiledShaders_, 0, 1,
			core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
			"Writes compiled shaders to disk for faster loading on subsequent loads (enabling does not result in retrospective writing)");
		ADD_CVAR_REF("shader_write_merged_source", writeMergedSource_, writeMergedSource_, 0, 1,
			core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
			"Writes the merged shader source to file before it's compiled");
	//	ADD_CVAR_REF("shader_asyncCompile", asyncShaderCompile_, asyncShaderCompile_, 0, 1,
	//		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
	//		"Performs hardware shader compiling async");
		ADD_CVAR_REF("shader_stall_help_with_work", helpWithWorkOnShaderStall_, helpWithWorkOnShaderStall_, 0, 1,
			core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
			"When waiting for another thread to finish compiling a shader, help process worker jobs");

		ADD_CVAR_REF("shader_compile_debug", compileDebug_, compileDebug_, 0, 1,
			core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
			"Shader compile debug logging");
	}

} // namespace shader

X_NAMESPACE_END