#include "stdafx.h"
#include "AssetDB.h"

#include <Containers\Array.h>
#include <Hashing\crc32.h>

#include <IFileSys.h>

X_LINK_LIB("engine_SqLite")

X_NAMESPACE_BEGIN(assetDb)

const char* AssetDB::DB_NAME = X_ENGINE_NAME"_asset.db";
const char* AssetDB::RAW_FILES_FOLDER = "raw_files/";


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
	if (!db_.execute("CREATE TABLE IF NOT EXISTS file_ids ("
		" file_id INTEGER PRIMARY KEY,"
		"name TEXT COLLATE NOCASE," // names are not unique since we allow same name for diffrent type.
		"path TEXT,"
		"args TEXT,"
		"type INTEGER,"
		"raw_file INTEGER,"
		"add_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,"
		"lastUpdateTime TIMESTAMP"
		");")) {
		X_ERROR("AssetDB", "Failed to create 'file_ids' table");
		return false;
	}

	if (!db_.execute("CREATE TABLE IF NOT EXISTS raw_files ("
		"file_id INTEGER PRIMARY KEY,"
		"path TEXT,"
		"size INTEGER,"
		"hash INTEGER,"
		"add_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL"
		");")) {
		X_ERROR("AssetDB", "Failed to create 'raw_files' table");
		return false;
	}


	return true;
}

bool AssetDB::DropTables(void)
{
	if (!db_.execute("DROP TABLE IF EXISTS raw_files_link;")) {
		return false;
	}
	if (!db_.execute("DROP TABLE IF EXISTS gdt_files;")) {
		return false;
	}
	if (!db_.execute("DROP TABLE IF EXISTS raw_files;")) {
		return false;
	}

	return true;
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

AssetDB::Result::Enum AssetDB::UpdateAsset(AssetType::Enum type, const core::string& name, core::Array<uint8_t>& data,
	const core::string& pathOpt, const core::string& argsOpt)
{
	int32_t assetId, rawId;

	if (!AssetExsists(type, name, &assetId)) {
		// add it?
		if (!AddAsset(type, name)) {
			return Result::NOT_FOUND;
		}
	}

	// work out crc for the data.
	core::Crc32* pCrc32 = gEnv->pCore->GetCrc32();
	uint32_t crc32 = pCrc32->GetCRC32(data.ptr(), data.size());

	rawId = std::numeric_limits<uint32_t>::max();

	if (data.isNotEmpty())
	{
		RawFile rawData;

		if (GetRawfileForId(assetId, rawData, &rawId))
		{
			// same?
			if (rawData.hash == crc32) { 
				return Result::UNCHANGED;
			}
		}
	}

	// start the transaction.
	sql::SqlLiteTransaction trans(db_);
	core::string stmt;

	// ok we have updated rawdata.
	if (data.isNotEmpty())
	{
		// save the file.
		{
			core::XFileScoped file;
			core::fileModeFlags mode;
			core::Path<char> filePath;

			mode.Set(core::fileMode::WRITE);
			mode.Set(core::fileMode::RECREATE);

			filePath = RAW_FILES_FOLDER;
			filePath /= AssetTypeRawFolder(type);
			filePath.ensureSlash();
			filePath /= name;

			gEnv->pFileSys->createDirectoryTree(filePath.c_str());

			if (!file.openFile(filePath.c_str(), mode)) {
				return Result::ERROR;
			}

			if (file.write(data.ptr(), data.size()) != data.size()) {
				return Result::ERROR;
			}
		}


		if (rawId == std::numeric_limits<uint32_t>::max())
		{
			sql::SqlLiteDb::RowId lastRowId;

			// insert entry
			{
				sql::SqlLiteCmd cmd(db_, "INSERT INTO raw_files (path, hash) VALUES(?,?)");
				cmd.bind(1, name.c_str());
				cmd.bind(2, static_cast<int32_t>(crc32));

				sql::Result::Enum res = cmd.execute();
				if (res != sql::Result::OK) {
					return Result::ERROR;
				}

				lastRowId = db_.lastInsertRowid();
			}

			// update asset row.
			{
				sql::SqlLiteCmd cmd(db_, "UPDATE file_ids SET raw_file = ? WHERE file_id = ?");
				cmd.bind(1, safe_static_cast<int32_t, sql::SqlLiteDb::RowId>(lastRowId));
				cmd.bind(2, assetId);

				sql::Result::Enum res = cmd.execute();
				if (res != sql::Result::OK) {
					return Result::ERROR;
				}
			}
		}
		else
		{
			// just update.
			sql::SqlLiteCmd cmd(db_, "UPDATE raw_files SET path = ?, size = ?, hash = ?, add_time = DateTime('now') WHERE file_id = ?");
			cmd.bind(1, name.c_str());
			cmd.bind(2, safe_static_cast<int32_t, size_t>(data.size()));
			cmd.bind(3, static_cast<int32_t>(crc32));
			cmd.bind(4, rawId);

			sql::Result::Enum res = cmd.execute();
			if (res != sql::Result::OK) {
				return Result::ERROR;
			}
		}
	}
	else
	{
		// humm


	}

	// now update file info.
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
 

bool AssetDB::AssetExsists(AssetType::Enum type, const core::string& name, int32_t* pId)
{
	sql::SqlLiteTransaction trans(db_, true);
	sql::SqlLiteQuery qry(db_, "SELECT file_id, name, type FROM file_ids WHERE type = ? AND name = ?");
	qry.bind(1, type);
	qry.bind(2, name.c_str());

	const auto it = qry.begin();

	if (it == qry.end()) {
		return false;
	}

	if (pId){
		*pId = (*it).get<int32_t>(0);
	}

	return true;
}


bool AssetDB::GetRawfileForId(int32_t assetId, RawFile& dataOut, int32_t* pId)
{
	// we get the raw_id from the asset.
	// and get the data.
	sql::SqlLiteTransaction trans(db_, true);
	sql::SqlLiteQuery qry(db_, "SELECT raw_files.file_id, raw_files.path, raw_files.hash FROM raw_files "
		"INNER JOIN file_ids on raw_files.file_id = file_ids.raw_file WHERE file_ids.file_id = ?");
	qry.bind(1, assetId);

	const auto it = qry.begin();

	if (it == qry.end()) {
		return false;
	}

	if (pId) {
		*pId = (*it).get<int32_t>(0);
	}

	dataOut.path = (*it).get< const char*>(1);
	dataOut.hash = static_cast<uint32_t>((*it).get<int32_t>(2));
	return true;
}

const char* AssetDB::AssetTypeRawFolder(AssetType::Enum type)
{
	return AssetType::ToString(type);
}


X_NAMESPACE_END