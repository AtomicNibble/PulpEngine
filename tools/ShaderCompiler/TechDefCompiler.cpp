#include "stdafx.h"
#include "TechDefCompiler.h"

X_NAMESPACE_BEGIN(engine)

namespace compiler
{
	TechDefCompiler::TechDefCompiler(core::MemoryArenaBase* arena) :
		arena_(arena),
		techDefs_(arena),
		shaderMan_(arena)
	{
		compileFlags_ |= render::shader::CompileFlag::OptimizationLvl3;
		compileFlags_ |= render::shader::CompileFlag::TreatWarningsAsErrors;
	}

	TechDefCompiler::~TechDefCompiler()
	{
		techDefs_.clearIncSrcCache();
		shaderMan_.shutDown();
	}


	void TechDefCompiler::PrintBanner(void)
	{
		X_LOG0("TechCompiler", "=================== V0.1 ===================");

	}

	bool TechDefCompiler::Init(void)
	{
		// max comp.
		shaderMan_.getBin().setCompressionLvl(core::Compression::CompressLevel::HIGH);

		return true;
	}

	void TechDefCompiler::setCompileFlags(render::shader::CompileFlags flags)
	{
		compileFlags_ = flags;
	}

	bool TechDefCompiler::CompileAll(void)
	{
		X_LOG0("TechCompiler", "Compiling all techs...");

		for (int32_t i = 0; i < MaterialCat::ENUM_COUNT; i++) {
			if (!Compile(static_cast<MaterialCat::Enum>(i))) {
				X_ERROR("TechCompiler", "Failed to compile cat: \"%s\"", MaterialCat::ToString(i));
				return false;
			}
		}

		return true;
	}

	bool TechDefCompiler::Compile(MaterialCat::Enum cat)
	{
		// so we want to get all techs for given cat and compile.

		engine::TechSetDefs::CatTypeArr types(arena_);
		if (!engine::TechSetDefs::getTechCatTypes(cat, types)) {
			X_ERROR("TechCompiler", "Failed to get tech cat type");
			return false;
		}

		for (const auto& tech : types)
		{
			if (!Compile(cat, tech)) {
				X_ERROR("TechCompiler", "Failed to compile cat: \"%s\" tech: \"%s\"", MaterialCat::ToString(cat), tech.c_str());
				return false;
			}
		}

		return true;
	}


	bool TechDefCompiler::Compile(MaterialCat::Enum cat, const core::string& techName)
	{
		// now we need the techDef.
		engine::TechSetDef* pTechDef = nullptr;
		if (!techDefs_.getTechDef(cat, techName.c_str(), pTechDef)) {
			X_ERROR("TechCompiler", "Failed to get tech def for cat:  \"%s\" tech: \"%s\"", MaterialCat::ToString(cat), techName.c_str());
			return false;
		}

		// for each tech.
		for (auto it = pTechDef->techBegin(); it != pTechDef->techEnd(); ++it)
		{
			// get the shaders.
			const Technique& tech = it->second; 

			using namespace render::shader;
			
			for (uint32_t i = 0; i < ShaderType::ENUM_COUNT - 1; i++)
			{
				const auto type = static_cast<ShaderType::Enum>(i);
				const auto stage = staderTypeToStageFlag(type);

				if (!tech.stages.IsSet(stage)) {
					continue;
				}

				const auto& shader = tech.shaders[type];

				//  make sure the shader in the vertex slot is a vertex shader etc..
				X_ASSERT(shader.type == type, "Incorrect shader type for stage index.")(shader.type, type);
			
				IShaderSource* pShaderSource = shaderMan_.sourceforName(shader.source);
				if (!pShaderSource) {
					X_ERROR("TechCompiler", "Failed to get shader source for compiling: \"%s\"", shader.source.c_str());
					return false;
				}

		

				PermatationFlags permFlags = Permatation::VertStreams;

				XHWShader* pHWShader = shaderMan_.createHWShader(type, shader.entry, pShaderSource, permFlags);
				if (!pHWShader) {
					X_ERROR("TechCompiler", "Failed to create HWShader for compiling: \"%s\"", shader.source.c_str());
					return false;
				}

				if (!shaderMan_.compileShader(pHWShader, compileFlags_))
				{
					X_ERROR("TechCompiler", "Failed to compile HWShader", shader.source.c_str());
					return false;
				}

				// we don't release the hwshader, so another tech that uses same source / flags
				// won't need to bother compiling.
			}
		}

		return true;
	}

	// ----------------------------------------------------

	bool TechDefCompiler::Clean(MaterialCat::Enum cat)
	{

		return true;
	}


	bool TechDefCompiler::CleanAll(void)
	{
		X_LOG0("TechCompiler", "Cleaning all compiled techs");

		for (int32_t i = 0; i < MaterialCat::ENUM_COUNT; i++) {
			if (!Clean(static_cast<MaterialCat::Enum>(i))) {
				return false;
			}
		}

		return true;
	}


} // namespace compiler

X_NAMESPACE_END