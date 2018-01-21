#pragma once

#include <IAssetDb.h>

#include "StackString.h"

X_NAMESPACE_BEGIN(core)


class AssetPath : public StackString<256, char>
{
	typedef StackString<256, char> BaseType;

public:
	static const size_t BUF_SIZE = MAX_PATH;
	
	static const char ASSET_NAME_SLASH = assetDb::ASSET_NAME_SLASH;
	static const char ASSET_NAME_INVALID_SLASH = assetDb::ASSET_NAME_INVALID_SLASH;

public:
	AssetPath();
	AssetPath(const AssetPath& oth);
	template<size_t Size>
	explicit AssetPath(const core::StackString<Size, char>& oth);

	explicit AssetPath(const char* const str);
	AssetPath(const char* const beginInclusive, const char* const endExclusive);

public:
	void replaceSeprators(void);

	bool stripAssetFolder(assetDb::AssetType::Enum type);

};

X_NAMESPACE_END

#include "AssetName.inl"