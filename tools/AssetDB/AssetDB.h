#pragma once

#include "../SqLite/SqlLib.h"


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
	X_DECLARE_ENUM(AssetType)(MODEL, ANIM, MATERIAL);
	X_DECLARE_ENUM(Result)(
		OK, 
		NOT_FOUND,
		NAME_TAKEN,
		UNCHANGED,
		ERROR
	);

public:
	AssetDB();
	~AssetDB();

	bool OpenDB(void);
	void CloseDB(void);
	bool CreateTables(void);
	bool DropTables(void);

public:
	Result::Enum AddAsset(AssetType::Enum type, const core::string& name);
	Result::Enum DeleteAsset(AssetType::Enum type, const core::string& name);
	Result::Enum RenameAsset(AssetType::Enum type, const core::string& name,
		const core::string& newName);

	Result::Enum UpdateAsset(AssetType::Enum type, const core::string& name, 
		core::Array<uint8_t>& data, const core::string& argsOpt);

	bool AssetExsists(AssetType::Enum type, const core::string& name, int32_t* pId = nullptr);

	bool GetArgsForAsset(int32_t id, core::string& argsOut);

private:
	bool GetRawfileForId(int32_t assetId, RawFile& dataOut, int32_t* pId = nullptr);

	static const char* AssetTypeRawFolder(AssetType::Enum type);
	static void AssetPathForName(AssetType::Enum type, const core::string& name, core::Path<char>& pathOut);

private:
	sql::SqlLiteDb db_;
	bool open_;
};

X_NAMESPACE_END