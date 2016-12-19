

X_NAMESPACE_BEGIN(engine)


typename Shader::AliaseArr::size_type TechDef::getNumAliases(void) const
{
	return aliases_.size();
}

typename const Shader::AliaseArr& TechDef::getAliases(void) const
{
	return aliases_;
}



X_NAMESPACE_END