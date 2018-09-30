#pragma once

#include "../SqLite/SqlLib.h"

#include <Util\Delegate.h>
#include <Hashing\sha1.h>
#include <Hashing\xxHash.h>
#include <Containers\Array.h>

#include <IAssetDb.h>
#include <ICompression.h>

X_NAMESPACE_DECLARE(core,
                    class LinearAllocator);

X_NAMESPACE_BEGIN(assetDb)

class DLL_EXPORT AssetDB
{
    // Version 1: stores raw files with hash after file name on disk.
    //				In order to migrate must rename all rawfiles on disk.
    // Version 2: correct compression algo enum, as the order changed
    //				In order to migrate must modify all raw asset compression headers to map to new algo enum.
    // Version 3: compressed buffer header changed
    // Version 4: change asset thumb hash to sha1
    // Version 5: set thumb width, height coloums to NOT NULL
    // Version 6: Change asset+args hash to xxHash64
public:
    static const int32_t DB_VERSION = 6;

    static const char* ASSET_DB_FOLDER;
    static const char* DB_NAME;
    static const char* CACHE_DB_NAME;
    static const char* RAW_FILES_FOLDER;
    static const char* THUMBS_FOLDER;

    typedef core::Hash::xxHash64Val RawFileHash;
    typedef core::Hash::SHA1Digest ThumbHash;

private:
    struct RawFile
    {
        int32_t file_id;
        int32_t size;
        core::string path;
        RawFileHash hash;
    };

public:
    struct ThumbInfo
    {
        X_INLINE ThumbInfo();
        X_INLINE ThumbInfo(ThumbId id, int32_t fileSize, Vec2i thumbDim, Vec2i srcDim, ThumbHash& hash);

        ThumbId id;
        int32_t fileSize;
        Vec2i thumbDim; // thumb dim
        Vec2i srcDim;   // dim of src, might be zero
        ThumbHash hash;
    };

    struct Mod
    {
        X_INLINE Mod() = default;
        X_INLINE Mod(ModId modId, core::string, core::Path<char>& path);
        X_INLINE Mod(ModId modId, const char* pName, const char* pOutDir);

        ModId modId;
        core::string name;
        core::Path<char> outDir;
    };

    // add type?
    struct AssetInfo
    {
        X_INLINE AssetInfo();
        X_INLINE AssetInfo(AssetId id, AssetId parentId, ModId modId, const char* pName, AssetType::Enum type);
        X_INLINE AssetInfo(AssetId id, AssetId parentId, ModId modId, const core::string& name, AssetType::Enum type);

        AssetId id;
        AssetId parentId;
        AssetType::Enum type;
        ModId modId;
        core::string name;
    };

    struct AssetRef
    {
        X_INLINE AssetRef();
        X_INLINE AssetRef(int32_t id, AssetId toId, AssetId fromId);

        int32_t id;
        AssetId toId;
        AssetId fromId;
    };

public:
    typedef assetDb::AssetId AssetId;
    typedef assetDb::ModId ModId;
    typedef assetDb::ProfileId ProfileId;
    typedef assetDb::ThumbId ThumbId;

    static const ModId INVALID_MOD_ID = assetDb::INVALID_MOD_ID;
    static const AssetId INVALID_ASSET_ID = assetDb::INVALID_ASSET_ID;
    static const AssetId INVALID_RAWFILE_ID = assetDb::INVALID_RAWFILE_ID;
    static const ThumbId INVALID_THUMB_ID = assetDb::INVALID_THUMB_ID;

    typedef assetDb::AssetType AssetType;
    X_DECLARE_ENUM(Result)
    (
        OK,
        NOT_FOUND,
        NAME_TAKEN, // should maybe call this 'exsists' ?
        UNCHANGED,
        HAS_REFS,
        ERROR);

    typedef std::array<int32_t, AssetType::ENUM_COUNT> AssetTypeCountsArr;
    typedef core::Array<Mod> ModsArr;
    typedef core::Array<AssetInfo> AssetInfoArr;
    typedef core::Array<AssetRef> RefsArr;
    typedef core::Array<AssetId> AssetIdArr;
    typedef core::Array<uint8_t> DataArr;

    // callbacks.
    typedef core::Delegate<bool(ModId id, const core::string& name, const core::Path<char>& outDir)> ModDelegate;
    typedef core::Delegate<bool(AssetType::Enum, const core::string& name)> AssetDelegate;

    typedef sql::SqlLiteDb::ThreadMode ThreadMode;

public:
    AssetDB();
    ~AssetDB();

    // Startup / shutdown api
    bool OpenDB(ThreadMode::Enum threadMode);
    void CloseDB(void);
    bool CreateTables(void);
    bool DropTables(void);
    bool AddDefaultMods(void);
    bool AddDefaultProfiles(void);
    bool PerformMigrations(void);
    bool Chkdsk(bool updateDB = false);
    bool AddTestData(size_t numMods, const AssetTypeCountsArr& assetCounts);

    // Conversion Profile api.
    Result::Enum AddProfile(const core::string& name);
    Result::Enum AddProfile(const core::string& name, const core::string& data);
    bool ProfileExsists(const core::string& name, ProfileId* pProfileId = nullptr);
    bool SetProfileData(const core::string& name, const core::string& data);
    bool GetProfileData(const core::string& name, core::string& dataOut);

    // Mod api
    Result::Enum AddMod(const core::string& name, const core::Path<char>& outDir);
    bool SetMod(const core::string& name); // must exsists.
    bool SetMod(ModId id);
    bool ModExsists(const core::string& name, ModId* pModId = nullptr);
    bool SetModPath(const core::string& name, const core::Path<char>& outDir);
    bool SetModPath(ModId modId, const core::Path<char>& outDir);
    ModId GetModId(const core::string& name);
    ModId GetcurrentModId(void) const;
    bool GetModInfo(ModId id, Mod& modOut);

    // mod specific counts
    bool GetAssetTypeCounts(ModId modId, AssetTypeCountsArr& countsOut);
    bool GetAssetTypeCount(ModId modId, AssetType::Enum type, int32_t& countOut);
    bool GetAssetList(ModId modId, AssetType::Enum type, AssetInfoArr& assetsOut);

    // all asset of type in every mod.
    bool GetAssetList(AssetType::Enum type, AssetInfoArr& assetsOut);

    // global counts
    bool GetNumAssets(int32_t& numOut);
    bool GetNumAssets(AssetType::Enum type, int32_t& numOut);
    bool GetNumAssets(ModId modId, int32_t& numOut);

    // global listing (to engine log)
    bool ListAssets(void);
    bool ListAssets(AssetType::Enum type);

    // get list of mods
    bool GetModsList(ModsArr& arrOut);

    // asset iteration
    bool IterateMods(ModDelegate func);
    bool IterateAssets(AssetDelegate func);
    bool IterateAssets(ModId modId, AssetDelegate func);
    bool IterateAssets(ModId modId, AssetType::Enum type, AssetDelegate func);
    bool IterateAssets(AssetType::Enum type, AssetDelegate func);

    // AddAsset with grouped transation, trans is not just touched, just required to make sure you call it with one.
    Result::Enum AddAsset(const sql::SqlLiteTransaction& trans, ModId modId, AssetType::Enum type, const core::string& name, AssetId* pId = nullptr);

    // none batched Add/delete/rename api
    Result::Enum AddAsset(AssetType::Enum type, const core::string& name, AssetId* pId = nullptr);
    Result::Enum AddAsset(ModId modId, AssetType::Enum type, const core::string& name, AssetId* pId = nullptr);
    Result::Enum DeleteAsset(AssetType::Enum type, const core::string& name);
    Result::Enum RenameAsset(AssetType::Enum type, const core::string& name, const core::string& newName);

    // Updating api
    Result::Enum UpdateAsset(AssetType::Enum type, const core::string& name, const DataArr& compressedData, const core::string& argsOpt);
    // will do the compression for you, saves losts of places duplicating same compressino logic in code base if they don't already have the
    // data in a compressed form.
    Result::Enum UpdateAssetRawFile(AssetType::Enum type, const core::string& name, const DataArr& data, core::Compression::Algo::Enum algo, core::Compression::CompressLevel::Enum lvl = core::Compression::CompressLevel::NORMAL);
    Result::Enum UpdateAssetRawFile(AssetId assetId, const DataArr& data, core::Compression::Algo::Enum algo, core::Compression::CompressLevel::Enum lvl = core::Compression::CompressLevel::NORMAL);
    Result::Enum UpdateAssetRawFile(AssetType::Enum type, const core::string& name, const DataArr& compressedData);
    Result::Enum UpdateAssetRawFile(AssetId assetId, const DataArr& compressedData);
    Result::Enum UpdateAssetArgs(AssetType::Enum type, const core::string& name, const core::string& argsOpt);

    Result::Enum UpdateAssetThumb(AssetType::Enum type, const core::string& name, Vec2i thumbDim, Vec2i srcDim, core::span<const uint8_t> data, core::Compression::Algo::Enum algo, core::Compression::CompressLevel::Enum lvl = core::Compression::CompressLevel::NORMAL);
    Result::Enum UpdateAssetThumb(AssetId assetId, Vec2i thumbDim, Vec2i srcDim, core::span<const uint8_t> data, core::Compression::Algo::Enum algo, core::Compression::CompressLevel::Enum lvl = core::Compression::CompressLevel::NORMAL);
    Result::Enum UpdateAssetThumb(AssetType::Enum type, const core::string& name, Vec2i thumbDim, Vec2i srcDim, core::span<const uint8_t> compressedData);
    Result::Enum UpdateAssetThumb(AssetId assetId, Vec2i thumbDim, Vec2i srcDim, core::span<const uint8_t> compressedData);
    
    // Deletes all thumb data.
    bool CleanThumbs(void);

    // if you want to get a assets id use this.
    bool AssetExsists(AssetType::Enum type, const core::string& name, AssetId* pIdOut = nullptr, ModId* pModIdOut = nullptr);
    bool AssetExsists(AssetType::Enum type, const core::string& name, ModId modId, AssetId* pIdOut = nullptr);

    // Misc data / info retrival
    bool GetArgsForAsset(AssetId assetId, core::string& argsOut);
    bool GetArgsHashForAsset(AssetId assetId, RawFileHash& argsHashOut);
    bool GetModIdForAsset(AssetId assetId, ModId& modIdOut);
    bool GetRawFileDataForAsset(AssetId assetId, DataArr& dataOut);
    bool GetRawFileCompAlgoForAsset(AssetId assetId, core::Compression::Algo::Enum& algoOut);
    bool AssetHasRawFile(AssetId assetId, int32_t* pRawFileId = nullptr);
    bool AssetHasThumb(AssetId assetId);
    bool GetThumbForAsset(AssetId assetId, ThumbInfo& info, DataArr& thumbDataOut);
    bool GetTypeForAsset(AssetId assetId, AssetType::Enum& typeOut); // this could be removed, or made private as GetAssetInfoForAsset, provides same ability.
    bool GetAssetInfoForAsset(AssetId assetId, AssetInfo& infoOut);

    // compile data helper.
    bool GetCompileFileDataForAsset(AssetId assetId, DataArr& dataOut);

    bool MarkAssetsStale(ModId modId);
    bool IsAssetStale(AssetId assetId);
    bool OnAssetCompiled(AssetId assetId);

    // some assetRef stuff.
    bool GetAssetRefCount(AssetId assetId, uint32_t& refCountOut);
    bool IterateAssetRefs(AssetId assetId, core::Delegate<bool(int32_t)> func);
    bool GetAssetRefs(AssetId assetId, AssetIdArr& refsOut);     // returns a list of assets that refrence assetId
    bool GetAssetRefsFrom(AssetId assetId, AssetIdArr& refsOut); // returns a list of assets that assetId refrences.
    bool GetAssetRefsFrom(AssetId assetId, RefsArr& refsOut);    // returns a list of refs that assetId refrences.

    Result::Enum AddAssertRef(AssetId assetId, AssetId targetAssetId);
    Result::Enum RemoveAssertRef(AssetId assetId, AssetId targetAssetId);

    // parent
    bool AssetHasParent(AssetId assetId, AssetId* pParentId = nullptr);
    bool AssetIsParent(AssetId assetId); // check if this asset has a parent.
    Result::Enum SetAssetParent(AssetId assetId, AssetId parentAssetId);
    Result::Enum RemoveAssetParent(AssetId assetId);

    bool GetOutputPathForAsset(ModId modId, assetDb::AssetType::Enum assType, const core::string& name, core::Path<char>& pathOut);

    static void GetOutputPathForAssetType(assetDb::AssetType::Enum assType,
        const core::Path<char>& modPath, core::Path<char>& pathOut);
    static void GetOutputPathForAsset(assetDb::AssetType::Enum assType, const core::string& name,
        const core::Path<char>& modPath, core::Path<char>& pathOut);

    static void GetRelativeOutputPathForAsset(assetDb::AssetType::Enum assType, const core::string& name,
        core::Path<char>& pathOut);

private:
    Result::Enum UpdateAssetRawFileHelper(const sql::SqlLiteTransactionBase& trans, AssetType::Enum type, const core::string& name,
        AssetId assetId, int32_t rawId, const DataArr& compressedData, RawFileHash dataHash);

private:
    bool GetRawfileForId(AssetId assetId, RawFile& dataOut, int32_t* pRawFileId = nullptr);
    bool GetRawfileForRawId(int32_t rawFileId, RawFile& dataOut);
    bool GetThumbInfoForId(AssetId assetId, ThumbInfo& dataOut, ThumbId* pThumbId = nullptr);
    bool MergeArgs(AssetId assetId, core::string& argsInOut);
    bool getDBVersion(int32_t& versionOut);
    bool setDBVersion(int32_t version);
    bool isModSet(void) const;

    static RawFileHash getMergedHash(RawFileHash data, RawFileHash args, int32_t dataLen);

    static const char* AssetTypeRawFolder(AssetType::Enum type);
    static void AssetPathForName(AssetType::Enum type, const core::string& name, RawFileHash rawDataHash, core::Path<char>& pathOut);
    static void AssetPathForRawFile(const RawFile& raw, core::Path<char>& pathOut);
    static void RawFilePathForName(AssetType::Enum type, const core::string& name, core::Path<char>& pathOut);
    static void ThumbPathForThumb(const ThumbInfo& info, core::Path<char>& pathOut);
    static bool ValidName(const core::string& name);

    static bool InflateBuffer(core::MemoryArenaBase* scratchArena, core::span<const uint8_t> deflated, DataArr& inflated);
    static bool DeflateBuffer(core::MemoryArenaBase* scratchArena, core::span<const uint8_t> data, DataArr& deflated,
        core::Compression::Algo::Enum algo, core::Compression::CompressLevel::Enum lvl);

private:
    sql::SqlLiteDb db_;
    ModId modId_;
    int32_t dbVersion_;
    bool open_;
};

X_NAMESPACE_END

#include "AssetDB.inl"