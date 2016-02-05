#include "stdafx.h"
#include "AssetDB.h"

X_LINK_LIB("engine_SqLite")

X_NAMESPACE_BEGIN(assetDb)

const char* AssetDB::DB_NAME = X_ENGINE_NAME"_asset.db";


AssetDB::AssetDB()
{

}

AssetDB::~AssetDB()
{

}

bool AssetDB::OpenDB(void)
{
	return db_.connect(DB_NAME);
}

void AssetDB::CloseDB(void)
{
	db_.disconnect();
}

bool AssetDB::CreateTables(void)
{
	// we want to store a list of the assets.
	// so create a table which has every asset in it.

	if (!db_.execute("CREATE TABLE IF NOT EXISTS file_ids ("
		" file_id INTEGER PRIMARY KEY,"
		"name TEXT COLLATE NOCASE,"
		"path TEXT,"
		"type INTEGER,"
		"write_time INTEGER"
		");")) {
		return false;
	}



	return false;
}

bool AssetDB::DropTables(void)
{

	if (!db_.execute("DROP TABLE IF EXISTS gdt_files;")) {
		return false;
	}

	return true;
}


X_NAMESPACE_END