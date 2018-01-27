
X_NAMESPACE_BEGIN(engine)

namespace techset
{

X_INLINE BaseTechSetDef::TechniqueArr::size_type BaseTechSetDef::numTechs(void) const
{
	return techs_.size();
}

X_INLINE BaseTechSetDef::TechniqueArr::const_iterator BaseTechSetDef::techBegin(void) const
{
	return techs_.begin();
}

X_INLINE BaseTechSetDef::TechniqueArr::const_iterator BaseTechSetDef::techEnd(void) const
{
	return techs_.end();
}

X_INLINE BaseTechSetDef::ParamArr::size_type BaseTechSetDef::numParams(void) const
{
	return params_.size();
}

X_INLINE BaseTechSetDef::ParamArr::const_iterator BaseTechSetDef::paramBegin(void) const
{
	return params_.begin();
}

X_INLINE BaseTechSetDef::ParamArr::const_iterator BaseTechSetDef::paramEnd(void) const
{
	return params_.end();
}

X_INLINE BaseTechSetDef::TextureArr::size_type BaseTechSetDef::numTexture(void) const
{
	return textures_.size();
}

X_INLINE BaseTechSetDef::TextureArr::const_iterator BaseTechSetDef::textureBegin(void) const
{
	return textures_.begin();
}

X_INLINE BaseTechSetDef::TextureArr::const_iterator BaseTechSetDef::textureEnd(void) const
{
	return textures_.end();
}

X_INLINE BaseTechSetDef::SamplerArr::size_type BaseTechSetDef::numSampler(void) const
{
	return samplers_.size();
}

X_INLINE BaseTechSetDef::SamplerArr::const_iterator BaseTechSetDef::samplerBegin(void) const
{
	return samplers_.begin();
}

X_INLINE BaseTechSetDef::SamplerArr::const_iterator BaseTechSetDef::samplerEnd(void) const
{
	return samplers_.end();
}

X_INLINE bool BaseTechSetDef::allSamplersAreStatic(void) const
{
	return allSamplersStatic_;
}

X_INLINE bool BaseTechSetDef::anySamplersAreStatic(void) const
{
	return anySamplersStatic_;
}



} // namespace techset


X_NAMESPACE_END
