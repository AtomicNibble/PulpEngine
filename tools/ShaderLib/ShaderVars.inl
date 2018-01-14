
X_NAMESPACE_BEGIN(render)

namespace shader
{

	X_INLINE bool ShaderVars::useCache(void) const
	{
		return useCache_ != 0;
	}

	X_INLINE bool ShaderVars::writeCompiledShaders(void) const
	{
		return writeCompiledShaders_ != 0;
	}

	X_INLINE bool ShaderVars::writeMergedSource(void) const
	{
		return writeMergedSource_ != 0;
	}

	X_INLINE bool ShaderVars::helpWithWorkOnShaderStall(void) const
	{
		return helpWithWorkOnShaderStall_ != 0;
	}

	X_INLINE bool ShaderVars::compileDebug(void) const
	{
		return compileDebug_ != 0;
	}

	X_INLINE void ShaderVars::setUseCache(bool use)
	{
		useCache_ = use ? 1 : 0;
	}

	X_INLINE void ShaderVars::setWriteCompiledShaders(bool write)
	{
		writeCompiledShaders_ = write ? 1 : 0;
	}


} // namespace shader

X_NAMESPACE_END