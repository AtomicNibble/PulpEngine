

X_NAMESPACE_BEGIN(core)

inline AssetPath::AssetPath()
{

}

inline AssetPath::AssetPath(const AssetPath& oth) :
	BaseType(oth)
{

}

template<size_t Size>
inline AssetPath::AssetPath(const core::StackString<Size, char>& oth)
{
	BaseType::append(oth.c_str(), oth.length());
}

inline AssetPath::AssetPath(const char* const str) :
	BaseType(str)
{

}

inline AssetPath::AssetPath(const char* const beginInclusive, const char* const endExclusive) :
	BaseType(beginInclusive, endExclusive)
{

}


inline void AssetPath::replaceSeprators(void)
{
	replaceAll(ASSET_NAME_INVALID_SLASH, ASSET_NAME_SLASH);
}

bool AssetPath::stripAssetFolder(assetDb::AssetType::Enum type)
{
	StackString<64, char> prefix(assetDb::AssetType::ToString(type));
	prefix.append('s', 1);
	prefix.append(ASSET_NAME_SLASH, 1);

	const char* pPrefix = BaseType::findCaseInsen(prefix.begin(), prefix.end());
	if (pPrefix)
	{
		BaseType::trimLeft(pPrefix + prefix.length());
		return true;
	}

	return false;
}


X_NAMESPACE_END
