
X_NAMESPACE_BEGIN(render)

namespace shader
{

	X_INLINE bool ShaderVars::writeCompiledShaders(void) const
	{
		return writeCompiledShaders_ != 0;
	}

	X_INLINE bool ShaderVars::writeMergedSource(void) const
	{
		return writeMergedSource_ != 0;
	}

	X_INLINE bool ShaderVars::asyncCompile(void) const
	{
		return asyncShaderCompile_ != 0;
	}


} // namespace shader

X_NAMESPACE_END