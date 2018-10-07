#pragma once


#include <IConverterModule.h>
#include <String\CmdArgs.h>
#include <Platform\Module.h>

#include <../SqLite/SqlLib.h>
#include <../AssetDB/AssetDB.h>

X_NAMESPACE_DECLARE(anim,
    struct IAnimLib)

X_NAMESPACE_DECLARE(model,
    struct IModelLib)

X_NAMESPACE_BEGIN(converter)

typedef assetDb::AssetDB::AssetType AssetType;

class Converter
    : public IConverterHost
{
    typedef core::traits::Function<void*(ICore* pSystem, const char* pModuleName)> ModuleLinkfunc;

    typedef std::array<IConverterModule*, AssetType::ENUM_COUNT> ConverterModuleInterfacesArr;

public:
    typedef IConverter::ConvertArgs ConvertArgs;
    typedef IConverter::OutPath OutPath;

public:
    CONVERTERLIB_EXPORT Converter(assetDb::AssetDB& db, core::MemoryArenaBase* scratchArea);
    CONVERTERLIB_EXPORT ~Converter();

    CONVERTERLIB_EXPORT void PrintBanner(void);
    CONVERTERLIB_EXPORT bool Init(const core::string& modName);

    // enables force conversion so assets are rebuilt even if not stale.
    CONVERTERLIB_EXPORT void forceConvert(bool force);
    CONVERTERLIB_EXPORT bool setConversionProfiles(const core::string& profileName);

    CONVERTERLIB_EXPORT bool Convert(AssetType::Enum assType, const core::string& name);
    CONVERTERLIB_EXPORT bool Convert(assetDb::ModId modId);
    CONVERTERLIB_EXPORT bool Convert(assetDb::ModId modId, AssetType::Enum assType);
    CONVERTERLIB_EXPORT bool Convert(AssetType::Enum assType);
    CONVERTERLIB_EXPORT bool ConvertAll(void);
    CONVERTERLIB_EXPORT bool CleanAll(const char* pMod = nullptr);
    CONVERTERLIB_EXPORT bool CleanAll(assetDb::ModId modId);

    CONVERTERLIB_EXPORT bool CleanThumbs(void);
    CONVERTERLIB_EXPORT bool GenerateThumbs(void); // generates thumbs for assets that don't have thumbs already and we support auto thumb generation.
    CONVERTERLIB_EXPORT bool Chkdsk(void);
    CONVERTERLIB_EXPORT bool Repack(void);

    // IConverterHost
    CONVERTERLIB_EXPORT bool GetAssetArgs(assetDb::AssetId assetId, ConvertArgs& args) X_FINAL;
    CONVERTERLIB_EXPORT bool GetAssetData(assetDb::AssetId assetId, DataArr& dataOut) X_FINAL;
    CONVERTERLIB_EXPORT bool GetAssetData(const char* pAssetName, AssetType::Enum assType, DataArr& dataOut) X_FINAL;
    CONVERTERLIB_EXPORT bool GetAssetDataCompAlgo(assetDb::AssetId assetId, core::Compression::Algo::Enum& algoOut) X_FINAL;
    CONVERTERLIB_EXPORT bool AssetExists(const char* pAssetName, assetDb::AssetType::Enum assType, assetDb::AssetId* pIdOut = nullptr) X_FINAL;
    CONVERTERLIB_EXPORT bool UpdateAssetThumb(assetDb::AssetId assetId, Vec2i thumbDim, Vec2i srcDim, core::span<const uint8_t> data, core::Compression::Algo::Enum algo, core::Compression::CompressLevel::Enum lvl) X_FINAL;
    CONVERTERLIB_EXPORT bool UpdateAssetThumb(assetDb::AssetId assetId, Vec2i thumbDim, Vec2i srcDim, core::span<const uint8_t> compressedData) X_FINAL;
    CONVERTERLIB_EXPORT bool UpdateAssetRawFile(assetDb::AssetId assetId, const DataArr& data, core::Compression::Algo::Enum algo, core::Compression::CompressLevel::Enum lvl) X_FINAL;

    CONVERTERLIB_EXPORT bool getConversionProfileData(assetDb::AssetType::Enum type, core::string& strOut) X_FINAL;

    CONVERTERLIB_EXPORT IConverter* GetConverter(assetDb::AssetType::Enum assType) X_FINAL;
    CONVERTERLIB_EXPORT physics::IPhysLib* GetPhsicsLib(void) X_FINAL;

    CONVERTERLIB_EXPORT core::MemoryArenaBase* getScratchArena(void) X_FINAL;
    // ~IConverterHost

private:
    bool CreateTables(void);

    bool MarkAssetsStale(assetDb::ModId modId);
    bool IsAssetStale(assetDb::AssetId assetId, assetDb::AssetDB::DataHash dataHash, assetDb::AssetDB::DataHash argsHash);
    bool OnAssetCompiled(assetDb::AssetId assetId, assetDb::AssetDB::DataHash& dataHashOut, assetDb::AssetDB::DataHash& argsHashOut);

    bool loadConversionProfiles(const core::string& profileName);
    void clearConversionProfiles(void);

    bool CleanMod(assetDb::AssetDB::ModId id, const core::string& name, const core::Path<char>& outDir);

    bool GenerateThumb(AssetType::Enum assType, const core::string& name);
    bool RepackAsset(AssetType::Enum assType, const core::string& name);
    bool Convert_int(AssetType::Enum assType, assetDb::AssetId assetId, ConvertArgs& args, const OutPath& pathOut);

    //	IConverter* GetConverter(AssetType::Enum assType);
    bool EnsureLibLoaded(AssetType::Enum assType);

    bool IntializeConverterModule(AssetType::Enum assType);
    bool IntializeConverterModule(AssetType::Enum assType, const char* dllName, const char* moduleClassName);
    void UnloadConverters(void);


private:
    core::MemoryArenaBase* scratchArea_;
    assetDb::AssetDB& db_;

    sql::SqlLiteDb cacheDb_;

    // physics converter is special just like you.
    physics::IPhysLib* pPhysLib_;
    IConverterModule* pPhysConverterMod_;

    IConverter* converters_[AssetType::ENUM_COUNT];
    core::string conversionProfiles_[AssetType::ENUM_COUNT];

    bool forceConvert_;
    bool _pad[3];

    ConverterModuleInterfacesArr converterModules_;
};

X_NAMESPACE_END