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

        void setCompileFlags(render::shader::CompileFlags flags);
        void setForceCompile(bool force);

        bool CompileAll(void);
        bool Compile(MaterialCat::Enum cat);
        bool Compile(MaterialCat::Enum cat, const core::string& techName);

        bool CleanAll(void);

    private:
        core::MemoryArenaBase* arena_;
        techset::TechSetDefs techDefs_;

        render::shader::CompileFlags compileFlags_;
        render::shader::XShaderManager shaderMan_;
    };

} // namespace compiler

X_NAMESPACE_END