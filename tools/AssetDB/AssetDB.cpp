#include "stdafx.h"
#include "AssetDB.h"



X_NAMESPACE_BEGIN(assetDb)

const char* AssetDB::DB_NAME = X_ENGINE_NAME"_asset.db";


AssetDB::AssetDB() : 
	db_(nullptr)
{

}

AssetDB::~AssetDB()
{
	CloseDB();
}

bool AssetDB::OpenDB(void)
{
	if (sqlite3_open(DB_NAME, &db_) == SQLITE_OK) {
		return true;
	}

	X_ERROR("AseetDB", "Failed to open db");
	return true;
}

void AssetDB::CloseDB(void)
{
	if (IsDbOpen()) {
		sqlite3_close(db_);
	}

	db_ = nullptr;
}

bool AssetDB::IsDbOpen(void) const
{
	return db_ != nullptr;
}

bool AssetDB::CreateTables(void)
{

	return false;
}


X_NAMESPACE_END