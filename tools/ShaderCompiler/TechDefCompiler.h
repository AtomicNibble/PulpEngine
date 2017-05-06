#pragma once


X_NAMESPACE_BEGIN(engine)

namespace compiler
{

	class TechDefCompiler
	{
	public:
		TechDefCompiler(core::MemoryArenaBase* arena);
		~TechDefCompiler();

		void PrintBanner(void);
		bool Init(void);

		bool CompileAll(void);
		bool Compile(MaterialCat::Enum cat);
		bool Compile(MaterialCat::Enum cat, const core::string& techName);

		bool Clean(MaterialCat::Enum cat);
		bool CleanAll(void);


	private:
		core::MemoryArenaBase* arena_;
		engine::TechSetDefs techDefs_;

		render::shader::XShaderManager shaderMan_;
	};

} // namespace compiler

X_NAMESPACE_END