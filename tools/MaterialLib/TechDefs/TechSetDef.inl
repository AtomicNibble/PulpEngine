
X_NAMESPACE_BEGIN(engine)

namespace techset
{

X_INLINE TechSetDef::TechniqueArr::size_type TechSetDef::numTechs(void) const
{
	return techs_.size();
}

X_INLINE TechSetDef::TechniqueArr::const_iterator TechSetDef::techBegin(void) const
{
	return techs_.begin();
}

X_INLINE TechSetDef::TechniqueArr::const_iterator TechSetDef::techEnd(void) const
{
	return techs_.end();
}

X_INLINE TechSetDef::ParamArr::size_type TechSetDef::numParams(void) const
{
	return params_.size();
}

X_INLINE TechSetDef::ParamArr::const_iterator TechSetDef::paramBegin(void) const
{
	return params_.begin();
}

X_INLINE TechSetDef::ParamArr::const_iterator TechSetDef::paramEnd(void) const
{
	return params_.end();
}

X_INLINE TechSetDef::TextureArr::size_type TechSetDef::numTexture(void) const
{
	return textures_.size();
}

X_INLINE TechSetDef::TextureArr::const_iterator TechSetDef::textureBegin(void) const
{
	return textures_.begin();
}

X_INLINE TechSetDef::TextureArr::const_iterator TechSetDef::textureEnd(void) const
{
	return textures_.end();
}

X_INLINE TechSetDef::SamplerArr::size_type TechSetDef::numSampler(void) const
{
	return samplers_.size();
}

X_INLINE TechSetDef::SamplerArr::const_iterator TechSetDef::samplerBegin(void) const
{
	return samplers_.begin();
}

X_INLINE TechSetDef::SamplerArr::const_iterator TechSetDef::samplerEnd(void) const
{
	return samplers_.end();
}

} // namespace techset


X_NAMESPACE_END
