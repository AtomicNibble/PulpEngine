
X_NAMESPACE_BEGIN(assetDb)

X_INLINE AssetDB::ThumbInfo::ThumbInfo() :
	id(INVALID_THUMB_ID),
	fileSize(0)
{

}

X_INLINE AssetDB::ThumbInfo::ThumbInfo(int32_t id_, int32_t fileSize_, Vec2i dimension_, core::Hash::MD5Digest& hash_) :
	id(id_),
	fileSize(fileSize_),
	dimension(dimension_),
	hash(hash_)
{

}

// -----------------------------------------------------


X_INLINE AssetDB::Mod::Mod(int32_t modId, core::string name_, core::Path<char>& outdir) :
	modId(modId),
	name(name_),
	outDir(outdir)
{

}

X_INLINE AssetDB::Mod::Mod(int32_t modId, const char* pName, const char* pOutDir) :
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

X_INLINE AssetDB::AssetInfo::AssetInfo(int32_t id_, int32_t parentId_, const char* pName, AssetType::Enum type_) :
	id(id_),
	parentId(parentId_),
	type(type_),
	name(pName)
{

}

X_INLINE AssetDB::AssetInfo::AssetInfo(int32_t id_, int32_t parentId_, const core::string& name_, AssetType::Enum type_) :
	id(id_),
	parentId(parentId_),
	type(type_),
	name(name_)
{

}


X_NAMESPACE_END