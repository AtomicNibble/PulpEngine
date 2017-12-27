#pragma once


#include <Containers\Array.h>
#include <IAssetPak.h>

#include <ICompression.h>

X_NAMESPACE_BEGIN(AssetPak)

X_DECLARE_FLAGS(PakBuilderFlag)(
	COMPRESSION,
	SHARED_DICT
);

typedef Flags<PakBuilderFlag> PakBuilderFlags;

typedef core::Array<uint8_t> DataVec;

struct Asset
{
	Asset(AssetId id, const core::string& name, AssetType::Enum type, DataVec&& data, core::MemoryArenaBase* arena);

	core::string name;
	AssetId id;
	AssetType::Enum type;

	size_t infaltedSize;
	DataVec data; // may be compressed.
};

struct SharedDict
{
	SharedDict(core::MemoryArenaBase* arena);

	size_t numSamples;
	DataVec dict;
};

struct CompressionOptions
{
	CompressionOptions() :
		enabled(false),
		maxRatio(0.95f),
		algo(core::Compression::Algo::STORE)
	{
	}

	bool enabled;
	float maxRatio;
	core::Compression::Algo::Enum algo;
};

class AssetPakBuilder
{
	typedef core::Array<Asset, core::ArrayAllocator<Asset>, core::growStrat::Multiply> AssetArr;
	typedef std::array<SharedDict*, AssetType::ENUM_COUNT> SharedDicArr;
	typedef std::array<CompressionOptions, AssetType::ENUM_COUNT> CompressionOptionsArr;


public:
	AssetPakBuilder(core::MemoryArenaBase* arena);
	~AssetPakBuilder();

	void setFlags(PakBuilderFlags flags);

	bool dumpMeta(core::Path<char>& pakPath);

	bool bake(void);
	bool save(core::Path<char>& path);

	void addAsset(AssetId id, const core::string& name, AssetType::Enum type, DataVec&& vec);

private:
	core::MemoryArenaBase* arena_;
	PakBuilderFlags flags_;

	AssetArr assets_;
	CompressionOptionsArr compression_;
	SharedDicArr dictonaries_;
};

X_NAMESPACE_END
