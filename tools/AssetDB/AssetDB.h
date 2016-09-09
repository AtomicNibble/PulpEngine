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
		int32_t modId;
		core::string name;
		core::Path<char> outDir;
	};


public:
	typedef int32_t ModId;

	static const ModId INVALID_MOD_ID = -1;

	typedef assetDb::AssetType AssetType;
	X_DECLARE_ENUM(Result)(
		OK, 
		NOT_FOUND,
		NAME_TAKEN,
		UNCHANGED,
		HAS_REFS,
		ERROR
	);

public:
	AssetDB();
	~AssetDB();

	bool OpenDB(void);
	void CloseDB(void);
	bool CreateTables(void);
	bool DropTables(void);
	bool AddDefaultMods(void);

	Result::Enum AddMod(const core::string& name, core::Path<char>& outDir);
	// must exsists.
	bool SetMod(const core::string& name);
	bool ModExsists(const core::string& name, ModId* pModId = nullptr);
	bool SetModPath(const core::string& name, const core::Path<char>& outDir);
	bool SetModPath(ModId modId, const core::Path<char>& outDir);
	ModId GetModId(const core::string& name);
	ModId GetcurrentModId(void) const;

	bool GetModInfo(ModId id, Mod& modOut);

public:
	bool IterateMods(core::Delegate<bool(ModId id, const core::string& name, core::Path<char>& outDir)> func);
	bool IterateAssets(core::Delegate<bool(AssetType::Enum, const core::string& name)> func);
	bool IterateAssets(ModId modId, core::Delegate<bool(AssetType::Enum, const core::string& name)> func);
	bool IterateAssets(AssetType::Enum type, core::Delegate<bool(AssetType::Enum, const core::string& name)> func);

	bool ListAssets(void);
	bool ListAssets(AssetType::Enum type);
	bool GetNumAssets(int32_t* pNumOut);
	bool GetNumAssets(AssetType::Enum type, int32_t* pNumOut);

	Result::Enum AddAsset(AssetType::Enum type, const core::string& name, int32_t* pId = nullptr);
	Result::Enum DeleteAsset(AssetType::Enum type, const core::string& name);
	Result::Enum RenameAsset(AssetType::Enum type, const core::string& name,
		const core::string& newName);

	Result::Enum UpdateAsset(AssetType::Enum type, const core::string& name, 
		core::Array<uint8_t>& data, const core::string& argsOpt);

	bool AssetExsists(AssetType::Enum type, const core::string& name, int32_t* pIdOut = nullptr, ModId* pModIdOut = nullptr);

	bool GetArgsForAsset(int32_t assetId, core::string& argsOut);
	bool GetArgsHashForAsset(int32_t idassetId, uint32_t& argsHashOut);
	bool GetModIdForAsset(int32_t idassetId, ModId& modIdOut);
	bool GetRawFileDataForAsset(int32_t assetId, core::Array<uint8_t>& dataOut);
	bool GetTypeForAsset(int32_t idassetId, AssetType::Enum& typeOut);

	// some assetRef stuff.
	bool GetAssetRefCount(int32_t assetId, uint32_t& refCountOut);
	bool IterateAssetRefs(int32_t assetId, core::Delegate<bool(int32_t)> func);
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

private:
	sql::SqlLiteDb db_;
	int32_t modId_;
	bool open_;
};

X_NAMESPACE_END