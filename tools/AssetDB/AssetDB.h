#pragma once

#include "../SqLite/SqlLib.h"

X_NAMESPACE_BEGIN(assetDb)


class DLL_EXPORT AssetDB
{
	static const char* DB_NAME;

public:
	X_DECLARE_ENUM(AssetType)(MODEL, ANIM);
	X_DECLARE_ENUM(Result)(
		OK, 
		NOT_FOUND,
		NAME_TAKEN,
		ERROR
	);

public:
	AssetDB();
	~AssetDB();

	Result::Enum AddAsset(AssetType::Enum type, const core::string& name);
	Result::Enum DeleteAsset(AssetType::Enum type, const core::string& name);
	Result::Enum RenameAsset(AssetType::Enum type, const core::string& name,
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