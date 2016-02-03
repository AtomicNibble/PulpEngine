#include "stdafx.h"
#include "AssetDB.h"

X_LINK_LIB("engine_SqLite")

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
	sql::SqlLiteCpp db;

	return db.connect(DB_NAME);

	/*int ret = 0;
	
	if (SQLITE_OK != (ret = sqlite3_initialize()))
	{
		X_ERROR("AseetDB", "Failed to initialize library: %d", ret);
		return false;
	}

	// open connection to a DB
	if (SQLITE_OK != (ret = sqlite3_open_v2(DB_NAME, &db_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL)))
	{
		X_ERROR("AseetDB", "Failed to open conn: %d", ret);
		return false;
	}

	return true; */
}

void AssetDB::CloseDB(void)
{
	if (IsDbOpen()) {
	//	sqlite3_close(db_);

//		sqlite3_shutdown();
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