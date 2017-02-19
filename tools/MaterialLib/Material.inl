X_NAMESPACE_BEGIN(engine)


X_INLINE Material::Material(core::MemoryArenaBase* arena) :
	techs_(arena),
	textures_(arena),
	pTechDefState_(nullptr)
{
	id_ = -1;

	surfaceType_ = MaterialSurType::NONE;
	polyOffsetType_ = MaterialPolygonOffset::NONE;
	mountType_ = MaterialMountType::NONE;

	usage_ = MaterialUsage::NONE;
	cat_ = MaterialCat::CODE;
}

X_INLINE Material::Tech* Material::getTech(core::StrHash hash, render::shader::VertexFormat::Enum vertFmt, PermatationFlags permFlags)
{
	core::Spinlock::ScopedLock lock(techLock_);

	for (auto& tech : techs_)
	{
		if (tech.hash == hash && tech.pPerm->vertFmt == vertFmt && tech.pPerm->permFlags == permFlags) {
			return &tech;
		}
	}

	return nullptr;
}

X_INLINE void Material::addTech(const Tech& tech)
{
	core::Spinlock::ScopedLock lock(techLock_);

	techs_.append(tech);
}

X_INLINE const int32_t Material::getID(void) const
{
	return id_;
}

X_INLINE void Material::setID(int32_t id)
{
	id_ = id;
}


X_INLINE void Material::setName(const core::string& name)
{
	name_ = name;
}

X_INLINE void Material::setName(const char* pName)
{
	name_ = pName;
}

X_INLINE void Material::setFlags(MaterialFlags flags)
{
	flags_ = flags;
}

X_INLINE void Material::setSurfaceType(MaterialSurType::Enum surfaceType)
{
	surfaceType_ = surfaceType;
}

X_INLINE void Material::setCoverage(MaterialCoverage::Enum coverage)
{
	coverage_ = coverage;
}

X_INLINE void Material::setPolyOffsetType(MaterialPolygonOffset::Enum polyOffset)
{
	polyOffsetType_ = polyOffset;
}

X_INLINE void Material::setMountType(MaterialMountType::Enum mt)
{
	mountType_ = mt;
}

X_INLINE void Material::setCat(MaterialCat::Enum cat)
{
	cat_ = cat;
}

X_INLINE void Material::setTechDefState(TechDefState* pTechDefState)
{
	pTechDefState_ = pTechDefState;
}

X_INLINE void Material::setTextures(const FixedTextureArr& texArr)
{
	textures_.resize(texArr.size());

	for (size_t i = 0; i < texArr.size(); i++)
	{
		textures_[i] = texArr[i];
	}
}

// ---------------------------------------------

X_INLINE bool Material::isDrawn(void) const
{
	return flags_.IsSet(MaterialFlag::NODRAW) == false;
}

X_INLINE bool Material::isLoaded(void) const
{
	return flags_.IsSet(MaterialFlag::LOAD_FAILED) == false;
}

X_INLINE bool Material::isDefault(void) const
{
	return flags_.IsSet(MaterialFlag::DEFAULT);
}

X_INLINE const core::string& Material::getName(void) const
{
	return name_;
}

X_INLINE MaterialFlags Material::getFlags(void) const
{
	return flags_;
}

X_INLINE MaterialSurType::Enum Material::getSurfaceType(void) const
{
	return surfaceType_;
}

X_INLINE MaterialCoverage::Enum Material::getCoverage(void) const
{
	return coverage_;
}

X_INLINE MaterialPolygonOffset::Enum Material::getPolyOffsetType(void) const
{
	return polyOffsetType_;
}

X_INLINE MaterialMountType::Enum Material::getMountType(void) const
{
	return mountType_;
}

X_INLINE MaterialCat::Enum Material::getCat(void) const
{
	return cat_;
}

X_INLINE TechDefState* Material::getTechDefState(void) const
{
	return pTechDefState_;
}


X_NAMESPACE_END
