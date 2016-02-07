#include "stdafx.h"
#include "AssetDB.h"

X_LINK_LIB("engine_SqLite")

X_NAMESPACE_BEGIN(assetDb)

const char* AssetDB::DB_NAME = X_ENGINE_NAME"_asset.db";


AssetDB::AssetDB() :
	modId_(-1)
{

}

AssetDB::~AssetDB()
{

}

bool AssetDB::AddAsset(AssetType::Enum type, const core::string& name)
{
	if (!isModSet()) {
		X_ERROR("AssetDB", "Mod must be set before calling AddAsset!");
		return false;
	}

	sql::SqlLiteTransaction trans(db_);
	sql::SqlLiteCmd cmd(db_, "INSERT INTO file_ids (name, type, mod_id) VALUES(?,?,?)");
	cmd.bind(1, name.c_str());
	cmd.bind(2, type);
	cmd.bind(3, modId_);

	int32_t res = cmd.execute();
	if (res != 0) {
		return false;
	}

	trans.commit();
	return true;
}


bool AssetDB::SetMod(const core::string& name)
{
	// does the mod exsist?
	{
		sql::SqlLiteTransaction trans(db_, true);
		sql::SqlLiteQuery qry(db_, "SELECT mod_id, name FROM mods WHERE name = ?");
		qry.bind(0, name.c_str());


		sql::SqlLiteQuery::iterator it = qry.begin();

		if (it != qry.end())
		{
			modId_ = (*it).get<int32_t>(0);

			X_LOG0("AssetDB", "Mod set: \"%s\" id: %i", name.c_str(), modId_);
			return true;
		}
	}

	{
		sql::SqlLiteTransaction trans(db_);
		sql::SqlLiteCmd cmd(db_, "INSERT INTO mods (name) VALUES(?)");
		cmd.bind(1, name.c_str());

		int32_t res = cmd.execute();
		if (res != 0) {
			return false;
		}

		trans.commit();
	}

	modId_ = safe_static_cast<int32_t, sql::SqlLiteDb::RowId>(db_.lastInsertRowid());
	X_LOG0("AssetDB", "Mod set: \"%s\" id: %i", name.c_str(), modId_);
	return true;
}

bool AssetDB::isModSet(void) const
{
	return modId_ >= 0;
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
	if (!db_.execute("CREATE TABLE IF NOT EXISTS mods ("
		" mod_id INTEGER PRIMARY KEY,"
		"name TEXT COLLATE NOCASE"
		");")) {
		return false;
	}

	if (!db_.execute("CREATE TABLE IF NOT EXISTS file_ids ("
		" file_id INTEGER PRIMARY KEY,"
		"name TEXT COLLATE NOCASE,"
		"path TEXT,"
		"type INTEGER,"
		"add_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,"
		"lastUpdateTime TIMESTAMP,"
		"mod_id INTEGER,"
		"FOREIGN KEY(mod_id) REFERENCES mods(mod_id)"
		");")) {
		return false;
	}


	return true;
}

bool AssetDB::DropTables(void)
{

	if (!db_.execute("DROP TABLE IF EXISTS gdt_files;")) {
		return false;
	}

	if (!db_.execute("DROP TABLE IF EXISTS mods;")) {
		return false;
	}

	return true;
}


X_NAMESPACE_END