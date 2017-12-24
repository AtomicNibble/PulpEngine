#pragma once


#include <Containers\Array.h>
#include <IAssetPak.h>

X_NAMESPACE_BEGIN(AssetPak)

typedef core::Array<uint8_t> DataVec;

struct Asset
{
	Asset(const core::string& name, AssetType::Enum type, DataVec&& data, core::MemoryArenaBase* arena);

	core::string name;
	AssetType::Enum type;

	DataVec data;
};


class AssetPakBuilder
{
	typedef core::Array<Asset, core::ArrayAllocator<Asset>, core::growStrat::Multiply> AssetArr;

public:
	AssetPakBuilder(core::MemoryArenaBase* arena);

	bool save(core::Path<char>& path);

	void addAsset(const core::string& name, AssetType::Enum type, DataVec&& vec);

private:
	core::MemoryArenaBase* arena_;

	AssetArr assets_;
};

X_NAMESPACE_END
