X_NAMESPACE_BEGIN(engine)

X_INLINE MaterialTech::MaterialTech(core::MemoryArenaBase* arena) :
	hashVal(0),
	pPerm(nullptr),
	pVariableState(nullptr),
	cbs(arena),
	materialCbs(arena),
	paramLinks(arena)
{

}

X_INLINE MaterialTech::MaterialTech(const MaterialTech& oth) :
	hashVal(oth.hashVal),
	pPerm(oth.pPerm),
	pVariableState(oth.pVariableState),
	cbs(oth.cbs),
	materialCbs(oth.materialCbs),
	paramLinks(oth.paramLinks)
{

}

X_INLINE MaterialTech::MaterialTech(MaterialTech&& oth) :
	hashVal(std::move(oth.hashVal)),
	pPerm(std::move(oth.pPerm)),
	pVariableState(std::move(oth.pVariableState)),
	cbs(std::move(oth.cbs)),
	materialCbs(std::move(oth.materialCbs)),
	paramLinks(std::move(oth.paramLinks))
{

}

X_INLINE MaterialTech& MaterialTech::operator=(const MaterialTech& oth)
{
	if (&oth != this)
	{
		hashVal = oth.hashVal;
		pPerm = oth.pPerm;
		pVariableState = oth.pVariableState;
		cbs = oth.cbs;
		materialCbs = oth.materialCbs;
		paramLinks = oth.paramLinks;
	}

	return *this;
}

X_INLINE MaterialTech& MaterialTech::operator=(MaterialTech&& oth)
{
	hashVal = std::move(oth.hashVal);
	pPerm = std::move(oth.pPerm);
	pVariableState = std::move(oth.pVariableState);
	cbs = std::move(oth.cbs);
	materialCbs = std::move(oth.materialCbs);
	paramLinks = std::move(oth.paramLinks);
	return *this;
}

// -------------------------------------------------------------------------------

X_INLINE Material::Material(core::string& name, core::MemoryArenaBase* arena) :
	core::AssetBase(name, assetDb::AssetType::MATERIAL),
	pTechDefState_(nullptr),
	techs_(arena),
	params_(arena),
	samplers_(arena),
	textures_(arena)
{
	id_ = -1;

	techs_.setGranularity(4);
	params_.setGranularity(4);
	samplers_.setGranularity(4);
	textures_.setGranularity(4);

	surfaceType_ = MaterialSurType::NONE;
	coverage_ = MaterialCoverage::OPAQUE;
	polyOffsetType_ = MaterialPolygonOffset::NONE;
	mountType_ = MaterialMountType::NONE;

	usage_ = MaterialUsage::NONE;
	cat_ = MaterialCat::CODE;
	status_ = core::LoadStatus::NotLoaded;
}

X_INLINE Material::Tech* Material::getTech(core::StrHash hash, render::shader::VertexFormat::Enum vertFmt, PermatationFlags permFlags)
{
	core::Spinlock::ScopedLock lock(techLock_);

	for (auto& tech : techs_)
	{
		if (tech.hashVal == hash && tech.pPerm->vertFmt == vertFmt && tech.pPerm->permFlags == permFlags) {
			return &tech;
		}
	}

	return nullptr;
}

X_INLINE void Material::addTech(Tech&& tech)
{
	core::Spinlock::ScopedLock lock(techLock_);

	techs_.append(std::forward<Tech>(tech));
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

X_INLINE void Material::setTextures(TextureArr&& textures)
{
	textures_ = std::move(textures);
}

X_INLINE void Material::setParams(ParamArr&& params)
{
	params_ = std::move(params);
}

X_INLINE void Material::setSamplers(SamplerArr&& samplers)
{
	samplers_ = std::move(samplers);
}

// ---------------------------------------------

X_INLINE bool Material::isDrawn(void) const
{
	return flags_.IsSet(MaterialFlag::NODRAW) == false;
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

X_INLINE const Material::ParamArr& Material::getParams(void) const
{
	return params_;
}

X_INLINE const Material::SamplerArr& Material::getSamplers(void) const
{
	return samplers_;
}

X_INLINE const Material::TextureArr& Material::getTextures(void) const
{
	return textures_;
}

X_NAMESPACE_END
