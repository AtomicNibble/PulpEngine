

X_NAMESPACE_BEGIN(render)

namespace shader
{
    X_INLINE void ShaderBin::setCompressionLvl(core::Compression::CompressLevel::Enum lvl)
    {
        compLvl_ = lvl;
    }

} // namespace shader

X_NAMESPACE_END