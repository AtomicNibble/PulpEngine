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

AssetDB::Result::Enum AssetDB::AddAsset(AssetType::Enum type, const core::string& name)
{
	if (AssetExsists(type, name)) {
		return Result::NAME_TAKEN;
	}

	sql::SqlLiteTransaction trans(db_);
	sql::SqlLiteCmd cmd(db_, "INSERT INTO file_ids (name, type) VALUES(?,?)");
	cmd.bind(1, name.c_str());
	cmd.bind(2, type);

	sql::Result::Enum res = cmd.execute();
	if (res != sql::Result::OK) {
		return Result::ERROR;
	}

	trans.commit();
	return Result::OK;
}


AssetDB::Result::Enum AssetDB::DeleteAsset(AssetType::Enum type, const core::string& name)
{
	sql::SqlLiteTransaction trans(db_);
	sql::SqlLiteCmd cmd(db_, "DELETE FROM file_ids WHERE type = ? AND name = ?");
	cmd.bind(1, type);
	cmd.bind(2, name.c_str());

	sql::Result::Enum res = cmd.execute();
	if (res != sql::Result::OK) {
		return Result::ERROR;
	}

	trans.commit();
	return Result::OK;
}


AssetDB::Result::Enum AssetDB::RenameAsset(AssetType::Enum type, const core::string& name,
	const core::string& newName)
{
	if (!AssetExsists(type, name)) {
		return Result::NOT_FOUND;
	}
	// check if asset of same type already has new name
	if (AssetExsists(type, newName)) {
		return Result::NAME_TAKEN; 
	}

	sql::SqlLiteTransaction trans(db_);
	sql::SqlLiteCmd cmd(db_, "UPDATE file_ids SET name = ? WHERE type = ? AND name = ?");
	cmd.bind(1, newName);
	cmd.bind(2, type);
	cmd.bind(3, name.c_str());

	sql::Result::Enum res = cmd.execute();
	if (res != sql::Result::OK) {
		return Result::ERROR;
	}

	trans.commit();
	return Result::OK;
}

AssetDB::Result::Enum AssetDB::UpdateAsset(AssetType::Enum type, const core::string& name,
	const core::string& pathOpt, const core::string& argsOpt)
{
	sql::SqlLiteTransaction trans(db_);

	core::string stmt;
	
	stmt = "UPDATE file_ids SET lastUpdateTime = DateTime('now')";

	if (pathOpt.isNotEmpty()) {
		stmt += ", path = :p";
	}
	if (argsOpt.isNotEmpty()) {
		stmt += ", args = :a";
	}
	
	stmt += " WHERE type = :t AND name = :n ";

	sql::SqlLiteCmd cmd(db_, stmt.c_str());
	if (pathOpt.isNotEmpty()) {
		cmd.bind(":p", pathOpt.c_str());
	}
	if (argsOpt.isNotEmpty()) {
		cmd.bind(":a", argsOpt.c_str());
	}

	cmd.bind(":t", type);
	cmd.bind(":n", name.c_str());

	sql::Result::Enum res = cmd.execute();
	if (res != sql::Result::OK) {
		return Result::ERROR;
	}

	trans.commit();
	return Result::OK;
}
 

bool AssetDB::AssetExsists(AssetType::Enum type, const core::string& name)
{
	sql::SqlLiteTransaction trans(db_, true);
	sql::SqlLiteQuery qry(db_, "SELECT name, type FROM file_ids WHERE type = ? AND name = ?");
	qry.bind(1, type);
	qry.bind(2, name.c_str());

	if (qry.begin() == qry.end()) {
		return false;
	}

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
		"name TEXT COLLATE NOCASE," // names are not unique since we allow same name for diffrent type.
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