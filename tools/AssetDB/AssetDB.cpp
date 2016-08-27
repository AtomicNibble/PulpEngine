#include "stdafx.h"
#include "AssetDB.h"

#include <Containers\Array.h>
#include <Hashing\crc32.h>

#include <IFileSys.h>
#include <ICompression.h>

#include <Compression\LZ4.h>
#include <Compression\Lzma2.h>
#include <Compression\Zlib.h>

#include <String\Json.h>


X_LINK_LIB("engine_SqLite")

X_NAMESPACE_BEGIN(assetDb)

const char* AssetDB::ASSET_DB_FOLDER = "asset_db";
const char* AssetDB::DB_NAME = X_ENGINE_NAME"_asset.db";
const char* AssetDB::RAW_FILES_FOLDER = "raw_files";


AssetDB::AssetDB() :
	modId_(-1),
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

	if(!CreateTables()) {
		return false;
	}

	if (!AddDefaultMods()) {
		return false;
	}

	// set the mod to base
	SetMod(core::string("base"));

	return true;
}

void AssetDB::CloseDB(void)
{
	open_ = false;
	db_.disconnect();
}

bool AssetDB::CreateTables(void)
{
	if (!db_.execute("CREATE TABLE IF NOT EXISTS mods ("
		"mod_id INTEGER PRIMARY KEY,"
		"name TEXT COLLATE NOCASE,"
		"out_dir TEXT"
		");")) {
		X_ERROR("AssetDB", "Failed to create 'mods' table");
		return false;
	}

	if (!db_.execute("CREATE TABLE IF NOT EXISTS file_ids ("
		" file_id INTEGER PRIMARY KEY,"
		"name TEXT COLLATE NOCASE," // names are not unique since we allow same name for diffrent type.
		"type INTEGER,"
		"args TEXT,"
		"argsHash INTEGER,"
		"raw_file INTEGER,"
		"add_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,"
		"lastUpdateTime TIMESTAMP,"
		"mod_id INTEGER,"
		"FOREIGN KEY(mod_id) REFERENCES mods(mod_id)"
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
	if (!db_.execute("DROP TABLE IF EXISTS mods;")) {
		return false;
	}

	return true;
}

bool AssetDB::AddDefaultMods(void)
{
	core::string core("core");
	core::string base("base");

	if (!ModExsists(core)) {
		AddMod(core, core::Path<char>("core_assets"));
	}
	if (!ModExsists(base)) {
		AddMod(base, core::Path<char>("base_assets"));
	}

	return true;
}


AssetDB::Result::Enum AssetDB::AddMod(const core::string& name, core::Path<char>& outDir)
{
	if (ModExsists(name)) {
		X_ERROR("AssetDB", "Mod with name \"%s\" already exists", name.c_str());
		return Result::NAME_TAKEN;
	}

	sql::SqlLiteTransaction trans(db_);
	sql::SqlLiteCmd cmd(db_, "INSERT INTO mods (name, out_dir) VALUES(?,?)");
	cmd.bind(1, name.c_str());
	cmd.bind(2, outDir.c_str());

	int32_t res = cmd.execute();
	if (res != 0) {
		return Result::ERROR;
	}

	trans.commit();
	return Result::OK;
}

bool AssetDB::SetMod(const core::string& name)
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

	X_ERROR("AssetDB", "Failed to set mod no mod called \"%s\" exsists", name.c_str());
	return false;
}

bool AssetDB::ModExsists(const core::string& name, ModId* pModId)
{
	sql::SqlLiteQuery qry(db_, "SELECT mod_id FROM mods WHERE name = ?");
	qry.bind(1, name.c_str());

	const auto it = qry.begin();

	if (it == qry.end()) {
		return false;
	}
	if (pModId) {
		*pModId = (*it).get<int32_t>(0);
	}

	return true;
}

bool AssetDB::SetModPath(const core::string& name, const core::Path<char>& outDir)
{
	ModId id;
	if (!ModExsists(name, &id)) {
		X_ERROR("AssetDB", "Failed to set mod id, mod \"%s\" don't exsist");
		return false;
	}

	return SetModPath(id, outDir);
}


bool AssetDB::SetModPath(ModId modId, const core::Path<char>& outDir)
{
	sql::SqlLiteTransaction trans(db_);
	sql::SqlLiteCmd cmd(db_, "UPDATE mods SET out_dir = ? WHERE mod_id = ?");
	cmd.bind(1, outDir.c_str());
	cmd.bind(2, modId);

	sql::Result::Enum res = cmd.execute();
	if (res != sql::Result::OK) {
		return false;
	}

	trans.commit();
	return true;
}

AssetDB::ModId AssetDB::GetModId(const core::string& name)
{
	ModId id;

	if (!ModExsists(name, &id)) {
		// tut tut!
		X_ERROR("AssetDB", "Mod \"%s\" is not a valid mod name", name.c_str());
		return INVALID_MOD_ID;
	}

	return id;
}

AssetDB::ModId AssetDB::GetcurrentModId(void) const
{
	return modId_;
}

bool AssetDB::GetModInfo(ModId id, Mod& modOut)
{
	sql::SqlLiteTransaction trans(db_, true);
	sql::SqlLiteQuery qry(db_, "SELECT out_dir, name FROM mods WHERE mod_id = ?");
	qry.bind(0, id);

	sql::SqlLiteQuery::iterator it = qry.begin();

	if (it != qry.end())
	{
		auto row = *it;
		const char* pOutPath = row.get<const char*>(0);
		const char* pName = row.get<const char*>(1);

		modOut.modId = id;
		modOut.outDir.set(pOutPath);
		modOut.name = pName;
		return true;
	}

	X_ERROR("AssetDB", "Failed to get mod dir for modId %" PRIi32, id);
	return false;
}



bool AssetDB::IterateMods(core::Delegate<bool(ModId id, const core::string& name, core::Path<char>& outDir)> func)
{
	sql::SqlLiteQuery qry(db_, "SELECT mod_id, name, out_dir, lastUpdateTime FROM mods");

	auto it = qry.begin();
	for (; it != qry.end(); ++it)
	{
		auto row = *it;

		const int32_t modId = row.get<int32_t>(0);
		const char* pName = row.get<const char*>(1);
		const char* pOutdir = row.get<const char*>(2);


		func.Invoke(static_cast<ModId>(modId), X_CONST_STRING(pName), core::Path<char>(pOutdir));
	}

	return true;
}


bool AssetDB::IterateAssets(core::Delegate<bool(AssetType::Enum, const core::string& name)> func)
{
	sql::SqlLiteQuery qry(db_, "SELECT name, type, lastUpdateTime FROM file_ids");

	auto it = qry.begin();
	for (; it != qry.end(); ++it)
	{
		auto row = *it;

		const char* pName = row.get<const char*>(0);
		const int32_t type = row.get<int32_t>(1);

		func.Invoke(static_cast<AssetType::Enum>(type), X_CONST_STRING(pName));
	}

	return true;
}

bool AssetDB::IterateAssets(ModId modId, core::Delegate<bool(AssetType::Enum, const core::string& name)> func)
{
	if (modId == INVALID_MOD_ID) {
		return false;
	}

	sql::SqlLiteQuery qry(db_, "SELECT name, type, lastUpdateTime FROM file_ids WHERE mod_id = ?");
	qry.bind(1, modId);

	auto it = qry.begin();
	for (; it != qry.end(); ++it)
	{
		auto row = *it;

		const char* pName = row.get<const char*>(0);
		const int32_t type = row.get<int32_t>(1);

		func.Invoke(static_cast<AssetType::Enum>(type), X_CONST_STRING(pName));
	}

	return true;
}

bool AssetDB::IterateAssets(AssetType::Enum type, core::Delegate<bool(AssetType::Enum, const core::string& name)> func)
{
	sql::SqlLiteQuery qry(db_, "SELECT name, lastUpdateTime FROM file_ids WHERE type = ?");
	qry.bind(1, type);

	auto it = qry.begin();
	for (; it != qry.end(); ++it)
	{
		auto row = *it;

		const char* pName = row.get<const char*>(0);

		func.Invoke(type, X_CONST_STRING(pName));
	}

	return true;
}

bool AssetDB::ListAssets(void)
{
	sql::SqlLiteQuery qry(db_, "SELECT name, type, lastUpdateTime FROM file_ids");

	auto it = qry.begin();
	for (; it != qry.end(); ++it)
	{
		auto row = *it;

		const char* pName = row.get<const char*>(0);
		const int32_t type = row.get<int32_t>(1);

		X_LOG0("AssetDB", "name: ^6%s^0 type: ^6%s", pName, AssetType::ToString(type));
	}
	
	return true;
}

bool AssetDB::ListAssets(AssetType::Enum type)
{
	sql::SqlLiteQuery qry(db_, "SELECT name, lastUpdateTime FROM file_ids WHERE type = ?");
	qry.bind(1, type);

	auto it = qry.begin();
	for (; it != qry.end(); ++it)
	{
		auto row = *it;

		const char* pName = row.get<const char*>(0);

		X_LOG0("AssetDB", "name: ^6%s^0", pName);
	}

	return true;
}


bool AssetDB::GetNumAssets(int32_t* pNumOut)
{
	X_ASSERT_NOT_NULL(pNumOut);
	sql::SqlLiteQuery qry(db_, "SELECT COUNT(*) FROM file_ids");

	auto it = qry.begin();
	if (it != qry.end())
	{
		auto row = *it;

		const int32_t count = row.get<int32_t>(0);

		*pNumOut = count;
		return true;
	}

	return false;
}

bool AssetDB::GetNumAssets(AssetType::Enum type, int32_t* pNumOut)
{
	X_ASSERT_NOT_NULL(pNumOut);
	sql::SqlLiteQuery qry(db_, "SELECT COUNT(*) FROM file_ids WHERE type = ?");
	qry.bind(1, type);

	auto it = qry.begin();
	if (it != qry.end())
	{
		auto row = *it;

		const int32_t count = row.get<int32_t>(0);

		*pNumOut = count;
		return true;
	}

	return false;
}


AssetDB::Result::Enum AssetDB::AddAsset(AssetType::Enum type, const core::string& name, int32_t* pId)
{
	if (AssetExsists(type, name)) {
		return Result::NAME_TAKEN;
	}
	if (!isModSet()) {
		X_ERROR("AssetDB", "Mod must be set before calling AddAsset!");
		return Result::ERROR;
	}

	sql::SqlLiteTransaction trans(db_);
	sql::SqlLiteCmd cmd(db_, "INSERT INTO file_ids (name, type, mod_id) VALUES(?,?,?)");
	cmd.bind(1, name.c_str());
	cmd.bind(2, type);
	cmd.bind(3, modId_);


	sql::Result::Enum res = cmd.execute();
	if (res != sql::Result::OK) {
		return Result::ERROR;
	}

	trans.commit();

	if (pId) {
		*pId = safe_static_cast<int32_t, sql::SqlLiteDb::RowId>(db_.lastInsertRowid());
	}

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


			// make sure dir tree for new name is valid.
			if (!gEnv->pFileSys->createDirectoryTree(newFilePath.c_str())) {
				X_ERROR("AssetDB", "Failed to create dir to move raw asset");
				return Result::ERROR;
			}


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

#if X_ENABLE_ASSERTIONS
	assetId = std::numeric_limits<int32_t>::max();
#endif // !X_ENABLE_ASSERTIONS

	if (!AssetExsists(type, name, &assetId)) {
		// add it?
		if (AddAsset(type, name, &assetId) != Result::OK) {
			X_ERROR("AssetDB", "Failed to add assert when trying to update a asset that did not exsists.");
			return Result::NOT_FOUND;
		}

		X_WARNING("AssetDB", "Added asset to db as it didnt exists when trying to update the asset");
	}

	X_ASSERT(assetId != std::numeric_limits<int32_t>::max(), "AssetId is invalid")();

	core::string args(argsOpt);

	if(!MergeArgs(assetId, args)) {
		X_WARNING("AssetDB", "Failed to merge args");
		return Result::ERROR;
	}

	core::Crc32* pCrc32 = gEnv->pCore->GetCrc32();
	const uint32_t dataCrc = pCrc32->GetCRC32(data.ptr(), data.size());
	const uint32_t argsCrc = pCrc32->GetCRC32(args.c_str(), args.length());

	bool dataChanged = true;

	rawId = std::numeric_limits<uint32_t>::max();
	if (data.isNotEmpty())
	{
		RawFile rawData;

		if (GetRawfileForId(assetId, rawData, &rawId))
		{
			if (rawData.hash == dataCrc) 
			{
				dataChanged = false;

				uint32_t argsHash;
				if (GetArgsHashForAsset(assetId, argsHash) && argsHash == argsCrc)
				{
					X_LOG0("AssetDB", "Skipping updates asset unchanged");
					return Result::UNCHANGED;
				}
			}
		}
	}

	// start the transaction.
	sql::SqlLiteTransaction trans(db_);
	core::string stmt;

	if (data.isNotEmpty() && dataChanged)
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

			// path include folder, so don't need type to load it.
			path = AssetTypeRawFolder(type);
			path.toLower();
			path.ensureSlash();
			path /= name;

			if (!gEnv->pFileSys->createDirectoryTree(filePath.c_str())) {
				X_ERROR("AssetDB", "Failed to create dir to save raw asset");
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

	// now update file info.
	stmt = "UPDATE file_ids SET lastUpdateTime = DateTime('now'), args = :args, argsHash = :argsHash";
	stmt += " WHERE type = :t AND name = :n ";

	sql::SqlLiteCmd cmd(db_, stmt.c_str());
	cmd.bind(":t", type);
	cmd.bind(":n", name.c_str());
	cmd.bind(":args", args.c_str());
	cmd.bind(":argsHash", static_cast<int32_t>(argsCrc));


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


bool AssetDB::GetArgsHashForAsset(int32_t assetId, uint32_t& argsHashOut)
{
	sql::SqlLiteQuery qry(db_, "SELECT argsHash FROM file_ids WHERE file_ids.file_id = ?");
	qry.bind(1, assetId);

	const auto it = qry.begin();

	if (it == qry.end()) {
		return false;
	}

	if ((*it).columnType(0) != sql::ColumType::SNULL) {
		argsHashOut = static_cast<uint32_t>((*it).get<int32_t>(0));
	}
	return true;
}

bool AssetDB::GetRawFileDataForAsset(int32_t assetId, core::Array<uint8_t>& dataOut)
{
	using namespace core::Compression;

	RawFile raw;

	if (!GetRawfileForId(assetId, raw)) {
		X_ERROR("AssetDB", "Failed to get rawfile info for data reterival");
		return false;
	}

	// load the file.
	core::XFileScoped file;
	core::fileModeFlags mode;
	core::Path<char> filePath;

	mode.Set(core::fileMode::READ);
	mode.Set(core::fileMode::SHARE);

	AssetPathForRawFile(raw, filePath);

	if (!file.openFile(filePath.c_str(), mode)) {
		X_ERROR("AssetDB", "Failed to open rawfile");
		return false;
	}

	size_t size = safe_static_cast<size_t, uint64_t>(file.remainingBytes());

	core::Array<uint8_t> compressedData(g_AssetDBArena);
	compressedData.resize(size);

	if (file.read(compressedData.ptr(), size) != size) {
		X_ERROR("AssetDB", "Failed to read rawfile contents");
		return false;
	}

	// decompress it.
	Algo::Enum algo = ICompressor::getAlgo(compressedData);

	if (algo == Algo::LZ4) {
		Compressor<LZ4> def;
		return def.inflate(g_AssetDBArena, compressedData, dataOut);
	}
	else if (algo == Algo::LZMA) {
		Compressor<LZMA> def;
		return def.inflate(g_AssetDBArena, compressedData, dataOut);
	}
	else if (algo == Algo::ZLIB) {
		Compressor<Zlib> def;
		return def.inflate(g_AssetDBArena, compressedData, dataOut);
	}

	X_ERROR("AssetDB", "Failed to read rawfile, unkown compression algo");
	return false;
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

	dataOut.file_id = (*it).get<int32_t>(0);
	dataOut.hash = static_cast<uint32_t>((*it).get<int32_t>(2));
	return true;
}

bool AssetDB::MergeArgs(int32_t assetId, core::string& argsInOut)
{
	sql::SqlLiteQuery qry(db_, "SELECT args FROM file_ids WHERE file_ids.file_id = ?");
	qry.bind(1, assetId);

	const auto it = qry.begin();

	if (it == qry.end()) {
		X_ERROR("AssetDB", "Failed to find asset for merging conversion args");
		return false;
	}

	if ((*it).columnType(0) == sql::ColumType::SNULL) {
		return true;
	}
	const char* pArgs = (*it).get<const char*>(0);
	if (!pArgs) {
		X_ERROR("AssetDB", "Asset args are null");
		return false;
	}

	size_t argsLen = core::strUtil::strlen(pArgs);
	if (argsLen < 1) {
		return true;
	}
	if (argsInOut.isEmpty()) {
		argsInOut = pArgs;
		return true;
	}

	core::json::Document dDb, dArgs(&dDb.GetAllocator());

	dDb.Parse(pArgs, argsLen);
	dArgs.Parse(argsInOut.c_str(), argsInOut.length());

	// we want to use the ones from args, and add in any from db.
	for (auto it = dDb.MemberBegin(); it != dDb.MemberEnd(); ++it)
	{
		if (dArgs.FindMember(it->name) == dArgs.MemberEnd()) {
			dArgs.AddMember(it->name, it->value, dArgs.GetAllocator());
		}
	}

	core::json::StringBuffer s;
	core::json::Writer<core::json::StringBuffer> writer(s);

	if (!dArgs.Accept(writer)) {
		X_ERROR("AssetDB", "Failed to generate merged conversion args");
		return false;
	}

	argsInOut = core::string(s.GetString(), s.GetSize());
	return true;
}

bool AssetDB::isModSet(void) const
{
	return modId_ >= 0 && modId_ != INVALID_MOD_ID;
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

void AssetDB::AssetPathForRawFile(const RawFile& raw, core::Path<char>& pathOut)
{
	pathOut = ASSET_DB_FOLDER;
	pathOut.ensureSlash();
	pathOut /= RAW_FILES_FOLDER;
	pathOut.ensureSlash();
	pathOut /= raw.path;
}




X_NAMESPACE_END