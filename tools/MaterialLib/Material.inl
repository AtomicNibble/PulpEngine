X_NAMESPACE_BEGIN(engine)

X_INLINE const core::string& Material::getName(void) const
{
	return name_;
}

X_INLINE void Material::setName(const char* pName)
{
	name_ = pName;
}

X_INLINE bool Material::isDrawn(void) const
{
	return flags_.IsSet(MaterialFlag::NODRAW) == false;
}

X_INLINE bool Material::isLoaded(void) const
{
	return flags_.IsSet(MaterialFlag::LOAD_FAILED) == false;
}


X_INLINE const MaterialFlags Material::getFlags(void) const
{
	return cat_;
}

X_INLINE void Material::setFlags(const MaterialFlags flags)
{
	flags_ = flags;
}

X_INLINE MaterialCat::Enum Material::getCat(void) const
{
	return cat_;
}

X_INLINE void Material::setCat(MaterialCat::Enum cat)
{
	cat_ = cat;
}

X_INLINE MaterialSurType::Enum Material::getSurfaceType(void) const
{
	return surfaceType_;
}

X_INLINE void Material::setSurfaceType(MaterialSurType::Enum surfaceType)
{
	surfaceType_ = surfaceType;
}


X_INLINE MaterialCullType::Enum Material::getCullType(void) const
{
	return cullType_;
}

X_INLINE void Material::setCullType(MaterialCullType::Enum cullType)
{
	cullType_ = cullType;
}

X_INLINE MaterialPolygonOffset::Enum Material::getPolyOffsetType(void) const
{
	return polyOffsetType_;
}

X_INLINE void Material::setPolyOffsetType(MaterialPolygonOffset::Enum polyOffsetType)
{
	polyOffsetType_ = polyOffsetType;
}

X_INLINE MaterialCoverage::Enum Material::getCoverage(void) const
{
	return coverage_;
}

X_INLINE void Material::setCoverage(MaterialCoverage::Enum coverage)
{
	coverage_ = coverage;
}




X_NAMESPACE_END
