
X_NAMESPACE_BEGIN(engine)


X_INLINE TechSetDef::TechniqueMap::size_type TechSetDef::numTechs(void) const
{
	return techs_.size();
}

X_INLINE TechSetDef::TechniqueMap::const_iterator TechSetDef::techBegin(void) const
{
	return techs_.begin();
}

X_INLINE TechSetDef::TechniqueMap::const_iterator TechSetDef::techEnd(void) const
{
	return techs_.end();
}

X_INLINE TechSetDef::ParamMap::size_type TechSetDef::numParams(void) const
{
	return params_.size();
}

X_INLINE TechSetDef::ParamMap::const_iterator TechSetDef::paramBegin(void) const
{
	return params_.begin();
}

X_INLINE TechSetDef::ParamMap::const_iterator TechSetDef::paramEnd(void) const
{
	return params_.end();
}

X_INLINE TechSetDef::SamplerMap::size_type TechSetDef::numSampler(void) const
{
	return samplers_.size();
}

X_INLINE TechSetDef::SamplerMap::const_iterator TechSetDef::samplerBegin(void) const
{
	return samplers_.begin();
}

X_INLINE TechSetDef::SamplerMap::const_iterator TechSetDef::samplerEnd(void) const
{
	return samplers_.end();
}

X_NAMESPACE_END
