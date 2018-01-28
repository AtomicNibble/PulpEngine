
X_NAMESPACE_BEGIN(engine)

namespace techset
{

X_INLINE BaseTechSetDef::TechniqueArr::size_type BaseTechSetDef::numTechs(void) const
{
	return techs_.size();
}

X_INLINE BaseTechSetDef::ParamArr::size_type BaseTechSetDef::numParams(void) const
{
	return params_.size();
}

X_INLINE BaseTechSetDef::TextureArr::size_type BaseTechSetDef::numTexture(void) const
{
	return textures_.size();
}

X_INLINE BaseTechSetDef::SamplerArr::size_type BaseTechSetDef::numSampler(void) const
{
	return samplers_.size();
}

X_INLINE BaseTechSetDef::TechniqueSpan BaseTechSetDef::getTechs(void) const
{
	return TechniqueSpan(techs_.begin(), techs_.end());
}

X_INLINE BaseTechSetDef::ParamSpan BaseTechSetDef::getParams(void) const
{
	return ParamSpan(params_.begin(), params_.end());
}

X_INLINE BaseTechSetDef::TextureSpan BaseTechSetDef::getTextures(void) const
{
	return TextureSpan(textures_.begin(), textures_.end());
}

X_INLINE BaseTechSetDef::SamplerSpan BaseTechSetDef::getSamplers(void) const
{
	return SamplerSpan(samplers_.begin(), samplers_.end());
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
