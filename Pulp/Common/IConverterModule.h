#pragma once

#ifndef X_CONVETER_MODULE_I_H_
#define X_CONVETER_MODULE_I_H_

#include <Extension\IEngineUnknown.h>
#include <Containers\Array.h>

#include <IAssetDb.h>
#include <ICompression.h>

X_NAMESPACE_DECLARE(anim,
    struct IAnimLib)

X_NAMESPACE_DECLARE(model,
    struct IModelLib)
X_NAMESPACE_DECLARE(physics,
    struct IPhysLib)

struct IConverter;
struct IConverterModule : public IEngineUnknown
{
    ENGINE_INTERFACE_DECLARE(IConverterModule, 0x695c4e33, 0x4481, 0x45a7, 0x91, 0xe8, 0xb6, 0x4b, 0x67, 0x13, 0xe1, 0x6d);

    virtual const char* GetName(void) X_ABSTRACT;

    virtual IConverter* Initialize(void) X_ABSTRACT;
    virtual bool ShutDown(IConverter* pCon) X_ABSTRACT;
};

struct AssetDep
{
    AssetDep() = default;
    AssetDep(assetDb::AssetType::Enum type, const core::string& name) : 
        type(type),
        name(name)
    {}

    bool operator==(AssetDep& oth) const {
        return type == oth.type && name == oth.name;
    }

    bool operator<(AssetDep& oth) const {
        return type < oth.type && name < oth.name;
    }

    assetDb::AssetType::Enum type;
    core::string name;
};

using AssetDepArr = core::Array<AssetDep>;

struct IConverterHost
{
    typedef core::Array<uint8_t> DataArr;
    typedef core::string ConvertArgs;

    virtual ~IConverterHost() = default;

    virtual bool GetAssetArgs(assetDb::AssetId assetId, ConvertArgs& args) X_ABSTRACT;
    virtual bool GetAssetData(assetDb::AssetId assetId, DataArr& dataOut) X_ABSTRACT;
    virtual bool GetAssetDataCompAlgo(assetDb::AssetId assetId, core::Compression::Algo::Enum& algoOut) X_ABSTRACT;
    virtual bool GetAssetData(const char* pAssetName, assetDb::AssetType::Enum assType, DataArr& dataOut) X_ABSTRACT;
    virtual bool AssetExists(const char* pAssetName, assetDb::AssetType::Enum assType, assetDb::AssetId* pIdOut = nullptr) X_ABSTRACT;
    virtual bool UpdateAssetThumb(assetDb::AssetId assetId, Vec2i thumbDim, Vec2i srcDim, core::span<const uint8_t> data, core::Compression::Algo::Enum algo, core::Compression::CompressLevel::Enum lvl) X_ABSTRACT;
    virtual bool UpdateAssetThumb(assetDb::AssetId assetId, Vec2i thumbDim, Vec2i srcDim, core::span<const uint8_t> compressedData) X_ABSTRACT;
    virtual bool UpdateAssetRawFile(assetDb::AssetId assetId, const DataArr& data, core::Compression::Algo::Enum algo, core::Compression::CompressLevel::Enum lvl) X_ABSTRACT;
    virtual bool SetDependencies(assetDb::AssetId assetId, core::span<AssetDep> dependencies) X_ABSTRACT;

    // get global conversion settings data.
    virtual bool getConversionProfileData(assetDb::AssetType::Enum type, core::string& strOut) X_ABSTRACT;

    virtual IConverter* GetConverter(assetDb::AssetType::Enum assType) X_ABSTRACT;

    // can return null.
    virtual physics::IPhysLib* GetPhsicsLib(void) X_ABSTRACT;

    virtual core::MemoryArenaBase* getScratchArena(void) X_ABSTRACT;
};

struct IConverter
{
    typedef core::Array<uint8_t> DataArr;
    typedef core::string ConvertArgs;
    typedef core::Path<char> OutPath;

    virtual ~IConverter() = default;

    // gets the file extension this converter outputs with.
    virtual const char* getOutExtension(void) const X_ABSTRACT;

    virtual bool Convert(IConverterHost& host, assetDb::AssetId assetId, ConvertArgs& args, const OutPath& destPath) X_ABSTRACT;

    // thumbs disabled for all types by default.
    virtual bool thumbGenerationSupported(void) const {
        return false;
    }
    virtual bool CreateThumb(IConverterHost& host, assetDb::AssetId assetId, Vec2i targetDim) {
        X_UNUSED(host, assetId, targetDim);
        X_ASSERT_UNREACHABLE();
        return false;
    }

    virtual bool repackSupported(void) const {
        return false;
    }

    virtual bool Repack(IConverterHost& host, assetDb::AssetId assetId) const {
        X_UNUSED(host, assetId);
        X_ASSERT_UNREACHABLE();
        return false;
    }
};

#endif // !X_CONVETER_MODULE_I_H_
