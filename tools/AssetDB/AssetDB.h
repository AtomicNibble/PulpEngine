#pragma once

#include "../SqLite/SqlLib.h"

#include <Util\Delegate.h>

#include <IAssetDb.h>

X_NAMESPACE_DECLARE(core,
	template<typename T>
	class Array;
);

X_NAMESPACE_BEGIN(assetDb)


class DLL_EXPORT AssetDB
{
	static const char* ASSET_DB_FOLDER;
	static const char* DB_NAME;
	static const char* RAW_FILES_FOLDER;

	struct RawFile
	{
		int32_t file_id;
		core::string path;
		uint32_t hash;
	};

public:
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
		X_INLINE AssetInfo(int32_t id, int32_t parentId, const char* pName);
		X_INLINE AssetInfo(int32_t id, int32_t parentId, const core::string& name);

		int32_t id;
		int32_t parentId;
		core::string name;
	};


public:
	typedef int32_t ModId;

	static const ModId INVALID_MOD_ID = -1;
	static const ModId INVALID_ASSET_ID = -1;

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

public:
	AssetDB();
	~AssetDB();

	bool OpenDB(void);
	void CloseDB(void);
	bool CreateTables(void);
	bool DropTables(void);
	bool AddDefaultMods(void);
	bool AddTestData(size_t numMods, const AssetTypeCountsArr& assetCounts);

	Result::Enum AddMod(const core::string& name, core::Path<char>& outDir);
	// must exsists.
	bool SetMod(const core::string& name);
	bool SetMod(ModId id);
	bool ModExsists(const core::string& name, ModId* pModId = nullptr);
	bool SetModPath(const core::string& name, const core::Path<char>& outDir);
	bool SetModPath(ModId modId, const core::Path<char>& outDir);
	ModId GetModId(const core::string& name);
	ModId GetcurrentModId(void) const;

	bool GetModInfo(ModId id, Mod& modOut);

	bool GetAssetTypeCounts(ModId modId, AssetTypeCountsArr& countsOut);
	bool GetAssetTypeCount(ModId modId, AssetType::Enum type, int32_t& countOut);
	bool GetAssetList(ModId modId, AssetType::Enum type, AssetInfoArr& assetsOut);

public:
	bool GetModsList(ModsArr& arrOut);
	bool IterateMods(core::Delegate<bool(ModId id, const core::string& name, core::Path<char>& outDir)> func);
	bool IterateAssets(core::Delegate<bool(AssetType::Enum, const core::string& name)> func);
	bool IterateAssets(ModId modId, core::Delegate<bool(AssetType::Enum, const core::string& name)> func);
	bool IterateAssets(AssetType::Enum type, core::Delegate<bool(AssetType::Enum, const core::string& name)> func);

	bool ListAssets(void);
	bool ListAssets(AssetType::Enum type);
	bool GetNumAssets(int32_t* pNumOut);
	bool GetNumAssets(AssetType::Enum type, int32_t* pNumOut);

	// AddAsset with grouped transation, trans is not just touched, just required to make sure you call it with one.
	Result::Enum AddAsset(const sql::SqlLiteTransaction& trans, AssetType::Enum type, const core::string& name, int32_t* pId = nullptr);
	Result::Enum AddAsset(AssetType::Enum type, const core::string& name, int32_t* pId = nullptr);
	Result::Enum AddAsset(ModId modId, AssetType::Enum type, const core::string& name, int32_t* pId = nullptr);
	Result::Enum DeleteAsset(AssetType::Enum type, const core::string& name);
	Result::Enum RenameAsset(AssetType::Enum type, const core::string& name,
		const core::string& newName);

	Result::Enum UpdateAsset(AssetType::Enum type, const core::string& name, 
		core::Array<uint8_t>& data, const core::string& argsOpt);
	Result::Enum UpdateAssetArgs(AssetType::Enum type, const core::string& name, const core::string& argsOpt);

	// if you want to get a assets id use this.
	bool AssetExsists(AssetType::Enum type, const core::string& name, int32_t* pIdOut = nullptr, ModId* pModIdOut = nullptr);

	bool GetArgsForAsset(int32_t assetId, core::string& argsOut);
	bool GetArgsHashForAsset(int32_t assetId, uint32_t& argsHashOut);
	bool GetModIdForAsset(int32_t assetId, ModId& modIdOut);
	bool GetRawFileDataForAsset(int32_t assetId, core::Array<uint8_t>& dataOut);
	bool GetTypeForAsset(int32_t assetId, AssetType::Enum& typeOut);
	bool GetAssetInfoForAsset(int32_t assetId, AssetInfo& infoOut);

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
	bool GetRawfileForId(int32_t assetId, RawFile& dataOut, int32_t* pId = nullptr);
	bool MergeArgs(int32_t assetId, core::string& argsInOut);
	bool isModSet(void) const;

	static const char* AssetTypeRawFolder(AssetType::Enum type);
	static void AssetPathForName(AssetType::Enum type, const core::string& name, core::Path<char>& pathOut);
	static void AssetPathForRawFile(const RawFile& raw, core::Path<char>& pathOut);
	static bool ValidName(const core::string& name);

private:
	sql::SqlLiteDb db_;
	int32_t modId_;
	bool open_;
};

X_NAMESPACE_END


#include "AssetDB.inl"