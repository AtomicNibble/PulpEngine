

X_NAMESPACE_BEGIN(render)

namespace shader
{
    X_INLINE ShaderVars& XShaderManager::getShaderVars(void)
    {
        return vars_;
    }

    X_INLINE ShaderBin& XShaderManager::getBin(void)
    {
        return shaderBin_;
    }

} // namespace shader

X_NAMESPACE_END