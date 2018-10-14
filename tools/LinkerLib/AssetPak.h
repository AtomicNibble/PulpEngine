#pragma once

#include <Containers\Array.h>
#include <Containers\HashMap.h>
#include <IAssetPak.h>

#include <ICompression.h>

#include <Hashing\xxHash.h>

X_NAMESPACE_BEGIN(AssetPak)

X_DECLARE_FLAGS(PakBuilderFlag)
(
    COMPRESSION,
    SHARED_DICT,
    HINT_MEMORY
);

typedef Flags<PakBuilderFlag> PakBuilderFlags;

typedef core::Array<uint8_t> DataVec;

typedef core::Hash::xxHash64Val AssetHash;

struct Asset
{
    Asset(AssetId id, AssetType::Enum type, const core::string& name, DataVec&& data, core::MemoryArenaBase* arena);
   
    // Disable copy
    Asset(const Asset&) = delete;
    Asset& operator=(const Asset&) = delete;
    Asset(Asset&&) = default;
    Asset& operator=(Asset&&) = default;

    core::string name;
    AssetId id;
    AssetType::Enum type;

    size_t infaltedSize;
    DataVec data; // may be compressed.

    AssetHash hash;
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
    typedef std::array<int32_t, AssetType::ENUM_COUNT> AssetCountArr;
    typedef core::HashMap<AssetId, bool> AssetIdHashMap;

public:
    AssetPakBuilder(core::MemoryArenaBase* arena);
    ~AssetPakBuilder();

    void setFlags(PakBuilderFlags flags);

    bool dumpMetaOS(core::Path<wchar_t>& osPath);

    bool process(void);
    bool save(const core::Path<char>& path);

    void addAsset(AssetId id, AssetType::Enum type, const core::string& name, DataVec&& vec);
    bool hasAsset(AssetId id) const;

private:
    core::MemoryArenaBase* arena_;
    PakBuilderFlags flags_;

    AssetArr assets_;
    AssetIdHashMap assetLookup_;

    CompressionOptionsArr compression_;
    SharedDicArr dictonaries_;
    AssetCountArr assetCounts_;
    AssetCountArr compressedAssetCounts_;

    uint64_t defaltedAssetSize_;
    uint64_t infaltedAssetSize_;
};

X_NAMESPACE_END
