#pragma once

#include "../SqLite/SqlLib.h"

#include <Util\Delegate.h>
#include <Hashing\MD5.h>

#include <IAssetDb.h>
#include <ICompression.h>

X_NAMESPACE_DECLARE(core,
	template<typename T>
	class Array;

	class LinearAllocator;
);

X_NAMESPACE_BEGIN(assetDb)


class DLL_EXPORT AssetDB
{
	static const char* ASSET_DB_FOLDER;
	static const char* DB_NAME;
	static const char* RAW_FILES_FOLDER;
	static const char* THUMBS_FOLDER;

	static const size_t MAX_COMPRESSOR_SIZE;

	struct RawFile
	{
		int32_t file_id;
		int32_t size;
		core::string path;
		uint32_t hash;
	};


public:
	struct ThumbInfo
	{
		X_INLINE ThumbInfo();
		X_INLINE ThumbInfo(int32_t id, int32_t fileSize, Vec2i thumbDim, Vec2i srcDim, core::Hash::MD5Digest& hash);

		int32_t id;
		int32_t fileSize;
		Vec2i thumbDim;	// thumb dim
		Vec2i srcDim;		// dim of src, might be zero
		core::Hash::MD5Digest hash;
	};

	struct Mod
	{
		X_INLINE Mod() = default;
		X_INLINE Mod(int32_t modId, core::string, core::Path<char>& path);
		X_INLINE Mod(int32_t modId, const char* pName, const char* pOutDir);

		int32_t modId;
		core::string name;
		core::Path<char> outDir;
	};

	// add type?
	struct AssetInfo
	{
		X_INLINE AssetInfo();
		X_INLINE AssetInfo(int32_t id, int32_t parentId, const char* pName, AssetType::Enum type);
		X_INLINE AssetInfo(int32_t id, int32_t parentId, const core::string& name, AssetType::Enum type);

		int32_t id;
		int32_t parentId;
		AssetType::Enum type;
		core::string name;
	};


public:
	typedef int32_t ModId;

	static const ModId INVALID_MOD_ID = -1;
	static const ModId INVALID_ASSET_ID = -1;
	static const ModId INVALID_RAWFILE_ID = -1;
	static const ModId INVALID_THUMB_ID = -1;

	typedef assetDb::AssetType AssetType;
	X_DECLARE_ENUM(Result)(
		OK, 
		NOT_FOUND,
		NAME_TAKEN,
		UNCHANGED,
		HAS_REFS,
		ERROR
	);

	typedef std::array<int32_t, AssetType::ENUM_COUNT> AssetTypeCountsArr;
	typedef core::Array<Mod> ModsArr;
	typedef core::Array<AssetInfo> AssetInfoArr;
	typedef core::Array<int32_t> AssetIdArr;
	typedef core::Array<uint8_t> DataArr;

	// callbacks.
	typedef core::Delegate<bool(ModId id, const core::string& name, core::Path<char>& outDir)> ModDelegate;
	typedef core::Delegate<bool(AssetType::Enum, const core::string& name)> AssetDelegate;

	typedef sql::SqlLiteDb::ThreadMode ThreadMode;

public:
	AssetDB();
	~AssetDB();

	// Startup / shutdown api
	bool OpenDB(ThreadMode::Enum threadMode = ThreadMode::SINGLE);
	void CloseDB(void);
	bool CreateTables(void);
	bool DropTables(void);
	bool AddDefaultMods(void);
	bool AddTestData(size_t numMods, const AssetTypeCountsArr& assetCounts);

	// Mod api
	Result::Enum AddMod(const core::string& name, core::Path<char>& outDir);	
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

	// global counts
	bool GetNumAssets(int32_t* pNumOut);
	bool GetNumAssets(AssetType::Enum type, int32_t* pNumOut);
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
	Result::Enum AddAsset(const sql::SqlLiteTransaction& trans, AssetType::Enum type, const core::string& name, int32_t* pId = nullptr);

	// none batched Add/delete/rename api
	Result::Enum AddAsset(AssetType::Enum type, const core::string& name, int32_t* pId = nullptr);
	Result::Enum AddAsset(ModId modId, AssetType::Enum type, const core::string& name, int32_t* pId = nullptr);
	Result::Enum DeleteAsset(AssetType::Enum type, const core::string& name);
	Result::Enum RenameAsset(AssetType::Enum type, const core::string& name, const core::string& newName);

	// Updating api
	Result::Enum UpdateAsset(AssetType::Enum type, const core::string& name, const DataArr& data, const core::string& argsOpt);
	// will do the compression for you, saves losts of places duplicating same compressino logic in code base if they don't already have the 
	// data in a compressed form.
	Result::Enum UpdateAssetRawFile(AssetType::Enum type, const core::string& name, const DataArr& data, core::Compression::Algo::Enum algo, core::Compression::CompressLevel::Enum lvl = core::Compression::CompressLevel::NORMAL);
	Result::Enum UpdateAssetRawFile(int32_t assetId, const DataArr& data, core::Compression::Algo::Enum algo, core::Compression::CompressLevel::Enum lvl = core::Compression::CompressLevel::NORMAL);
	Result::Enum UpdateAssetRawFile(AssetType::Enum type, const core::string& name, const DataArr& data);
	Result::Enum UpdateAssetRawFile(int32_t assetId, const DataArr& data);
	Result::Enum UpdateAssetArgs(AssetType::Enum type, const core::string& name, const core::string& argsOpt);
	Result::Enum UpdateAssetThumb(AssetType::Enum type, const core::string& name, Vec2i thumbDim, Vec2i srcDim, const DataArr& data);
	Result::Enum UpdateAssetThumb(int32_t assetId, Vec2i thumbDim, Vec2i srcDim, const DataArr& data);

	// if you want to get a assets id use this.
	bool AssetExsists(AssetType::Enum type, const core::string& name, int32_t* pIdOut = nullptr, ModId* pModIdOut = nullptr);

	// Misc data / info retrival
	bool GetArgsForAsset(int32_t assetId, core::string& argsOut);
	bool GetArgsHashForAsset(int32_t assetId, uint32_t& argsHashOut);
	bool GetModIdForAsset(int32_t assetId, ModId& modIdOut);
	bool GetRawFileDataForAsset(int32_t assetId, DataArr& dataOut);
	bool GetThumbForAsset(int32_t assetId, ThumbInfo& info, DataArr& thumbDataOut);
	bool GetTypeForAsset(int32_t assetId, AssetType::Enum& typeOut); // this could be removed, or made private as GetAssetInfoForAsset, provides same ability.
	bool GetAssetInfoForAsset(int32_t assetId, AssetInfo& infoOut);
	
	bool MarkAssetsStale(int32_t modId);
	bool IsAssetStale(int32_t assetId);
	bool OnAssetCompiled(int32_t assetId);

	// some assetRef stuff.
	bool GetAssetRefCount(int32_t assetId, uint32_t& refCountOut);
	bool IterateAssetRefs(int32_t assetId, core::Delegate<bool(int32_t)> func);
	bool GetAssetRefs(int32_t assetId, AssetIdArr& refsOut);
	Result::Enum AddAssertRef(int32_t assetId, int32_t targetAssetId);
	Result::Enum RemoveAssertRef(int32_t assetId, int32_t targetAssetId);

	// parent
	bool AssetHasParent(int32_t assetId, int32_t* pParentId = nullptr);
	bool AssetIsParent(int32_t assetId); // check if this asset has a parent.
	Result::Enum SetAssetParent(int32_t assetId, int32_t parentAssetIt);
	Result::Enum RemoveAssetParent(int32_t assetId);


private:
	bool GetRawfileForId(int32_t assetId, RawFile& dataOut, int32_t* pRawFileId = nullptr);
	bool GetThumbInfoForId(int32_t assetId, ThumbInfo& dataOut, int32_t* pThumbId = nullptr);
	bool MergeArgs(int32_t assetId, core::string& argsInOut);
	bool isModSet(void) const;

	static const char* AssetTypeRawFolder(AssetType::Enum type);
	static void AssetPathForName(AssetType::Enum type, const core::string& name, core::Path<char>& pathOut);
	static void AssetPathForRawFile(const RawFile& raw, core::Path<char>& pathOut);
	static void ThumbPathForThumb(const ThumbInfo& info, core::Path<char>& pathOut);
	static bool ValidName(const core::string& name);

	static core::Compression::ICompressor* AllocCompressor(core::LinearAllocator* pAllocator, core::Compression::Algo::Enum algo);

private:
	sql::SqlLiteDb db_;
	int32_t modId_;
	bool open_;
};

X_NAMESPACE_END


#include "AssetDB.inl"