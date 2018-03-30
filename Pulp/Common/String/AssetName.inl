

X_NAMESPACE_BEGIN(core)

inline AssetName::AssetName()
{

}

inline AssetName::AssetName(assetDb::AssetType::Enum type, const core::string& name)
{
	set(type, name);
}

inline AssetName::AssetName(assetDb::AssetType::Enum type, const core::string& name, const char* pExt)
{
	set(type, name);
	setExtension(pExt);
}

inline AssetName::AssetName(const AssetName& oth) :
	BaseType(oth)
{

}

template<size_t Size>
inline AssetName::AssetName(const core::StackString<Size, char>& oth)
{
	BaseType::append(oth.c_str(), oth.length());
}

inline AssetName::AssetName(const char* const str) :
	BaseType(str)
{

}

inline AssetName::AssetName(const char* const beginInclusive, const char* const endExclusive) :
	BaseType(beginInclusive, endExclusive)
{

}


inline void AssetName::replaceSeprators(void)
{
	replaceAll(ASSET_NAME_INVALID_SLASH, ASSET_NAME_SLASH);
}

inline void AssetName::set(assetDb::AssetType::Enum type, const core::string& name)
{
	BaseType::set(assetDb::AssetType::ToString(type));
	BaseType::append('s', 1);
	BaseType::append(assetDb::ASSET_NAME_SLASH, 1);
	BaseType::toLower();
	BaseType::append(name.begin(), name.end());
}

inline bool AssetName::stripAssetFolder(assetDb::AssetType::Enum type)
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


inline const char* AssetName::extension(bool incDot) const
{
	const char* res = BaseType::findLast('.');

	if (!res) {
		return BaseType::begin();
	}

	if (incDot) {
		return res;
	}
	return res + 1;
}

inline void AssetName::setExtension(const char* pExtension)
{
	const char* remove = BaseType::findLast('.');	// need to remvoe a extension?
	bool has_dot = (pExtension[0] == '.'); // new extension got a dot?
	bool is_blank = (pExtension[0] == '\0'); //

	if (remove) {
		BaseType::trimRight(remove);
	}

	if (!is_blank)
	{
		if (!has_dot) {
			BaseType::append('.', 1);
		}

		BaseType::append(pExtension);
	}
}


inline void AssetName::removeExtension(void)
{
	if (isNotEmpty()) {
		setExtension("");
	}
}

X_NAMESPACE_END
