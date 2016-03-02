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

bool AssetDB::AddAsset(AssetType::Enum type, const core::string& name)
{
	sql::SqlLiteTransaction trans(db_);
	sql::SqlLiteCmd cmd(db_, "INSERT INTO file_ids (name, type) VALUES(?,?)");
	cmd.bind(1, name.c_str());
	cmd.bind(2, type);

	int32_t res = cmd.execute();
	if (res != 0) {
		return false;
	}

	trans.commit();
	return true;
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
	if (!db_.execute("CREATE TABLE IF NOT EXISTS file_ids ("
		" file_id INTEGER PRIMARY KEY,"
		"name TEXT COLLATE NOCASE,"
		"path TEXT,"
		"args TEXT,"
		"type INTEGER,"
		"add_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,"
		"lastUpdateTime TIMESTAMP"
		");")) {
		X_ERROR("AssetDB", "Failed to create 'file_ids' table");
		return false;
	}


	return true;
}

bool AssetDB::DropTables(void)
{

	if (!db_.execute("DROP TABLE IF EXISTS gdt_files;")) {
		return false;
	}


	return true;
}


X_NAMESPACE_END