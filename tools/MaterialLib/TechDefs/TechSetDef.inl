
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

X_NAMESPACE_END
