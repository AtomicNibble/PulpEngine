
X_NAMESPACE_BEGIN(engine)


X_INLINE const int32_t XMaterial::getID(void) const
{
	return id_;
}

X_INLINE void XMaterial::setID(int32_t id)
{
	id_ = id;
}


X_INLINE void XMaterial::setName(const core::string& name)
{
	name_ = name;
}

X_INLINE void XMaterial::setName(const char* pName)
{
	name_ = pName;
}

X_INLINE void XMaterial::setFlags(MaterialFlags flags)
{
	flags_ = flags;
}

X_INLINE void XMaterial::setSurfaceType(MaterialSurType::Enum surfaceType)
{
	surfaceType_ = surfaceType;
}

X_INLINE void XMaterial::setCoverage(MaterialCoverage::Enum coverage)
{
	coverage_ = coverage;
}

X_INLINE void XMaterial::setPolyOffsetType(MaterialPolygonOffset::Enum polyOffset)
{
	polyOffsetType_ = polyOffset;
}

X_INLINE void XMaterial::setMountType(MaterialMountType::Enum mt)
{
	mountType_ = mt;
}

X_INLINE void XMaterial::setCat(MaterialCat::Enum cat)
{
	cat_ = cat;
}

X_INLINE void XMaterial::setStateDesc(render::StateDesc& stateDesc)
{
	stateDesc_ = stateDesc;
}

X_INLINE void XMaterial::setStateHandle(render::StateHandle handle)
{
	stateHandle_ = handle;
}


X_INLINE const core::string& XMaterial::getName(void) const
{
	return name_;
}

X_INLINE MaterialFlags XMaterial::getFlags(void) const
{
	return flags_;
}

X_INLINE MaterialSurType::Enum XMaterial::getSurfaceType(void) const
{
	return surfaceType_;
}

X_INLINE MaterialCoverage::Enum XMaterial::getCoverage(void) const
{
	return coverage_;
}

X_INLINE MaterialPolygonOffset::Enum XMaterial::getPolyOffsetType(void) const
{
	return polyOffsetType_;
}

X_INLINE MaterialMountType::Enum XMaterial::getMountType(void) const
{
	return mountType_;
}

X_INLINE MaterialCat::Enum XMaterial::getCat(void) const
{
	return cat_;
}

X_INLINE const render::StateDesc& XMaterial::getStateDesc(void) const
{
	return stateDesc_;
}

X_INLINE render::StateHandle XMaterial::getStateHandle(void) const
{
	return stateHandle_;
}

X_INLINE render::Commands::ResourceStateBase* XMaterial::getVariableState(void) const
{
	return pVariableState_;
}

X_NAMESPACE_END
