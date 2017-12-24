#pragma once


#include <Containers\Array.h>
#include <IAssetPak.h>

X_NAMESPACE_BEGIN(AssetPak)


struct Asset
{
	Asset(core::string& name, AssetType::Enum type, core::MemoryArenaBase* arena);

	core::string name;
	AssetType::Enum type;

	core::ByteStream data;
};


class AssetPakBuilder
{
	typedef core::Array<Asset, core::ArrayAllocator<Asset>, core::growStrat::Multiply> AssetArr;

public:
	AssetPakBuilder(core::MemoryArenaBase* arena);

	bool save(core::Path<char>& path);

	void addAsset(core::string& name, AssetType::Enum type);

private:
	core::MemoryArenaBase* arena_;

	AssetArr assets_;
};

X_NAMESPACE_END
