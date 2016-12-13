

X_NAMESPACE_BEGIN(engine)


typename TechDef::BoundTexturesArr::size_type TechDef::getNumBoundTextures(void) const
{
	return boundTextures_.size();
}

typename const TechDef::BoundTexturesArr& TechDef::getBoundTextures(void) const
{
	return boundTextures_;
}



X_NAMESPACE_END