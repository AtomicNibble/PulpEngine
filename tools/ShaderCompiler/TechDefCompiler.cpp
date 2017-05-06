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

		return true;
	}


	bool TechDefCompiler::CompileAll(void)
	{
		X_LOG0("TechCompiler", "Compiling all techs...");

		for (int32_t i = 0; i < MaterialCat::ENUM_COUNT; i++) {
			if (!Compile(static_cast<MaterialCat::Enum>(i))) {
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
				return false;
			}
		}

		return true;
	}


	bool TechDefCompiler::Compile(MaterialCat::Enum cat, const core::string& techName)
	{

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