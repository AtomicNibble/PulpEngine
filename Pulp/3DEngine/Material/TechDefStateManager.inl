

X_NAMESPACE_BEGIN(engine)

typename techset::Shader::AliaseArr::size_type TechDef::getNumAliases(void) const
{
    return aliases_.size();
}

typename const techset::Shader::AliaseArr& TechDef::getAliases(void) const
{
    return aliases_;
}

X_NAMESPACE_END