
X_NAMESPACE_BEGIN(assetDb)

X_INLINE AssetDB::ThumbInfo::ThumbInfo() :
	id(INVALID_THUMB_ID),
	fileSize(0)
{

}

X_INLINE AssetDB::ThumbInfo::ThumbInfo(int32_t id_, int32_t fileSize_, Vec2i thumbDim_, Vec2i srcDim_, core::Hash::MD5Digest& hash_) :
	id(id_),
	fileSize(fileSize_),
	thumbDim(thumbDim_),
	srcDim(srcDim_),
	hash(hash_)
{

}

// -----------------------------------------------------


X_INLINE AssetDB::Mod::Mod(ModId modId, core::string name_, core::Path<char>& outdir) :
	modId(modId),
	name(name_),
	outDir(outdir)
{

}

X_INLINE AssetDB::Mod::Mod(ModId modId, const char* pName, const char* pOutDir) :
	modId(modId),
	name(pName),
	outDir(pOutDir)
{

}

// -----------------------------------------------------


X_INLINE AssetDB::AssetInfo::AssetInfo() :
	id(INVALID_ASSET_ID),
	parentId(INVALID_ASSET_ID)
{

}

X_INLINE AssetDB::AssetInfo::AssetInfo(AssetId id_, AssetId parentId_, const char* pName, AssetType::Enum type_) :
	id(id_),
	parentId(parentId_),
	type(type_),
	name(pName)
{

}

X_INLINE AssetDB::AssetInfo::AssetInfo(AssetId id_, AssetId parentId_, const core::string& name_, AssetType::Enum type_) :
	id(id_),
	parentId(parentId_),
	type(type_),
	name(name_)
{

}

// -----------------------------------------------------

X_INLINE AssetDB::AssetRef::AssetRef() :
	id(INVALID_ASSET_ID),
	toId(INVALID_ASSET_ID),
	fromId(INVALID_ASSET_ID)
{

}

X_INLINE AssetDB::AssetRef::AssetRef(int32_t id, AssetId toId, AssetId fromId) :
	id(id),
	toId(toId),
	fromId(fromId)
{

}


X_NAMESPACE_END