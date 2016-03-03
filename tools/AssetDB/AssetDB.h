#pragma once

#include "../SqLite/SqlLib.h"

X_NAMESPACE_BEGIN(assetDb)


class AssetDB
{
	static const char* DB_NAME;

public:
	X_DECLARE_ENUM(AssetType)(MODEL, ANIM);

public:
	AssetDB();
	~AssetDB();

	bool AddAsset(AssetType::Enum type, const core::string& name);
	bool DeleteAsset(AssetType::Enum type, const core::string& name);
	bool RenameAsset(AssetType::Enum type, const core::string& name, 
		const core::string& newName);

	bool AssetExsists(AssetType::Enum type, const core::string& name);

	bool OpenDB(void);
	void CloseDB(void);
	bool CreateTables(void);
	bool DropTables(void);

private:

	sql::SqlLiteDb db_;
};

X_NAMESPACE_END