#include "stdafx.h"
#include "AssetDB.h"

#include <Containers\Array.h>
#include <Hashing\crc32.h>

#include <IFileSys.h>

X_LINK_LIB("engine_SqLite")

X_NAMESPACE_BEGIN(assetDb)

const char* AssetDB::ASSET_DB_FOLDER = "asset_db";
const char* AssetDB::DB_NAME = X_ENGINE_NAME"_asset.db";
const char* AssetDB::RAW_FILES_FOLDER = "raw_files";


AssetDB::AssetDB() :
	open_(false)
{

}

AssetDB::~AssetDB()
{

}


bool AssetDB::OpenDB(void)
{
	if (open_) {
		return true;
	}

	core::Path<char> dbPath;
	dbPath.append(ASSET_DB_FOLDER);
	dbPath.ensureSlash();
	dbPath.append(DB_NAME);


	if (!gEnv->pFileSys->createDirectoryTree(dbPath.c_str())) {
		X_ERROR("AssetDB", "Failed to create dir for asset_db");
		return false;
	}

	if (!db_.connect(dbPath.c_str())) {
		return false;
	}
	
	return CreateTables();
}

void AssetDB::CloseDB(void)
{
	open_ = false;
	db_.disconnect();
}

bool AssetDB::CreateTables(void)
{
	if (!db_.execute("CREATE TABLE IF NOT EXISTS file_ids ("
		" file_id INTEGER PRIMARY KEY,"
		"name TEXT COLLATE NOCASE," // names are not unique since we allow same name for diffrent type.
		"type INTEGER,"
		"args TEXT,"
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
	int32_t assetId;

	if (!AssetExsists(type, name, &assetId)) {
		return Result::NOT_FOUND;
	}
	// check if asset of same type already has new name
	if (AssetExsists(type, newName)) {
		return Result::NAME_TAKEN; 
	}

	// check for raw assets that need renaming to keep things neat.
	{
		RawFile rawData;
		int32_t rawId;
		if (GetRawfileForId(assetId, rawData, &rawId))
		{
			sql::SqlLiteTransaction trans(db_);
			sql::SqlLiteCmd cmd(db_, "UPDATE raw_files SET path = ? WHERE file_id = ?");
			cmd.bind(1, newName.c_str());
			cmd.bind(2, rawId);

			sql::Result::Enum res = cmd.execute();
			if (res != sql::Result::OK) {
				return Result::ERROR;
			}

			// now move the file.
			core::Path<char> newFilePath, oldFilePath;
			AssetPathForName(type, newName, newFilePath);
			AssetPathForName(type, rawData.path, oldFilePath);

			// if this fails, we return and the update is not commited.
			if (!gEnv->pFileSys->moveFile(oldFilePath.c_str(), newFilePath.c_str())) {
				X_ERROR("AssetDB", "Failed to move asset raw file");
				return Result::ERROR;
			}

			// we commit this even tho cmd below might fail, since the raw file has now moved..
			trans.commit();
		}

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
	core::Array<uint8_t>& data, const core::string& argsOpt)
{
	int32_t assetId, rawId;

	if (!AssetExsists(type, name, &assetId)) {
		// add it?
		if (AddAsset(type, name) != Result::OK) {
			X_ERROR("AssetDB", "Failed to add assert when trying to update a asset that did not exsists.");
			return Result::NOT_FOUND;
		}

		X_WARNING("AssetDB", "Added asset to db as it didnt exists when trying to update the asset");
	}

	// work out crc for the data.
	core::Crc32* pCrc32 = gEnv->pCore->GetCrc32();

	const uint32_t dataCrc = pCrc32->GetCRC32(data.ptr(), data.size());
	const uint32_t argsCrc = pCrc32->GetCRC32(argsOpt.c_str(), argsOpt.length());


	rawId = std::numeric_limits<uint32_t>::max();

	if (data.isNotEmpty())
	{
		RawFile rawData;

		if (GetRawfileForId(assetId, rawData, &rawId))
		{
			// same?
			if (rawData.hash == dataCrc) {
				X_LOG0("AssetDB", "Skipping updates asset unchanged");
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
		core::Path<char> path;

		{
			core::XFileScoped file;
			core::fileModeFlags mode;
			core::Path<char> filePath;

			mode.Set(core::fileMode::WRITE);
			mode.Set(core::fileMode::RECREATE);

			AssetPathForName(type, name, filePath);

			// set path as just the name.
			path = name;

			if (!gEnv->pFileSys->createDirectoryTree(filePath.c_str())) {
				X_ERROR("AssetDB", "Failed to create dir to save raw asset");
				return Result::ERROR;
			}

			// does the file already exsists?
			// how do we want to handle that?
			// for now error.
			if (gEnv->pFileSys->fileExists(filePath.c_str())) {
				X_ERROR("AssetDB", "Failed to write raw asset, file name taken");
				return Result::ERROR;
			}

			if (!file.openFile(filePath.c_str(), mode)) {
				X_ERROR("AssetDB", "Failed to write raw asset");
				return Result::ERROR;
			}

			if (file.write(data.ptr(), data.size()) != data.size()) {
				X_ERROR("AssetDB", "Failed to write raw asset data");
				return Result::ERROR;
			}
		}


		if (rawId == std::numeric_limits<uint32_t>::max())
		{
			sql::SqlLiteDb::RowId lastRowId;

			// insert entry
			{
				sql::SqlLiteCmd cmd(db_, "INSERT INTO raw_files (path, size, hash) VALUES(?,?,?)");
				cmd.bind(1, path.c_str());
				cmd.bind(2, safe_static_cast<int32_t, size_t>(data.size()));
				cmd.bind(3, static_cast<int32_t>(dataCrc));

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
			cmd.bind(1, path.c_str());
			cmd.bind(2, safe_static_cast<int32_t, size_t>(data.size()));
			cmd.bind(3, static_cast<int32_t>(dataCrc));
			cmd.bind(5, rawId);

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
	stmt = "UPDATE file_ids SET lastUpdateTime = DateTime('now'), args = :args";
	stmt += " WHERE type = :t AND name = :n ";

	sql::SqlLiteCmd cmd(db_, stmt.c_str());
	cmd.bind(":t", type);
	cmd.bind(":n", name.c_str());
	cmd.bind(":args", argsOpt.c_str());


	sql::Result::Enum res = cmd.execute();
	if (res != sql::Result::OK) {
		return Result::ERROR;
	}

	trans.commit();
	return Result::OK;
}
 

bool AssetDB::AssetExsists(AssetType::Enum type, const core::string& name, int32_t* pId)
{
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

bool AssetDB::GetArgsForAsset(int32_t assetId, core::string& argsOut)
{
	sql::SqlLiteQuery qry(db_, "SELECT args FROM file_ids WHERE file_ids.file_id = ?");
	qry.bind(1, assetId);

	const auto it = qry.begin();

	if (it == qry.end()) {
		return false;
	}

	if ((*it).columnType(0) != sql::ColumType::SNULL) {
		argsOut = (*it).get<const char*>(0);
	}
	else {
		argsOut.clear();
	}
	return true;
}

bool AssetDB::GetRawfileForId(int32_t assetId, RawFile& dataOut, int32_t* pId)
{
	// we get the raw_id from the asset.
	// and get the data.
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

	if ((*it).columnType(1) != sql::ColumType::SNULL) {
		dataOut.path = (*it).get<const char*>(1);
	}
	else {
		dataOut.path.clear();
	}

	dataOut.hash = static_cast<uint32_t>((*it).get<int32_t>(2));
	return true;
}

const char* AssetDB::AssetTypeRawFolder(AssetType::Enum type)
{
	return AssetType::ToString(type);
}

void AssetDB::AssetPathForName(AssetType::Enum type, const core::string& name, core::Path<char>& pathOut)
{
	pathOut = ASSET_DB_FOLDER;
	pathOut.ensureSlash();
	pathOut /= RAW_FILES_FOLDER;
	pathOut.ensureSlash();
	pathOut /= AssetTypeRawFolder(type);
	pathOut.toLower();
	pathOut.ensureSlash();
	pathOut /= name;
}


X_NAMESPACE_END