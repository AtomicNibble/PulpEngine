#include "stdafx.h"
#include "AssetDB.h"

#include <Containers\Array.h>
#include <Hashing\crc32.h>
#include <Hashing\MD5.h>

#include <IFileSys.h>
#include <ICompression.h>

#include <Compression\LZ4.h>
#include <Compression\Lzma2.h>
#include <Compression\Zlib.h>

#include <String\Json.h>
#include <String\HumanSize.h>

#include <Random\MultiplyWithCarry.h>

#include <Time\StopWatch.h>

#include <Memory/AllocationPolicies/LinearAllocator.h>

X_LINK_LIB("engine_SqLite")

X_NAMESPACE_BEGIN(assetDb)

namespace
{

	core::string randomAssetName(uint32_t assetNameLenMin, uint32_t assetNameLenMax)
	{
		static const char charSet[] =
			"0123456789"
			"_"
			"abcdefghijklmnopqrstuvwxyz";

		const size_t charSetNum = sizeof(charSet) - 1;
		const size_t len = core::random::MultiplyWithCarry(assetNameLenMin, assetNameLenMax);

		core::string name;
		name.reserve(len);

		bool addSlash = (core::random::MultiplyWithCarry() % 10) == 5;

		for (size_t i = 0; i < len; i++)
		{
			if (addSlash && i < (len - 2) && name.length() > 2 && (core::random::MultiplyWithCarry() % 5) == 1 &&
				name[i-1] != ASSET_NAME_SLASH) {
				name += ASSET_NAME_SLASH;
			}
			else {
				name += charSet[core::random::MultiplyWithCarry() % charSetNum];
			}
		}

		return name;
	}


} // namespace



// -----------------------------------------------------


const char* AssetDB::ASSET_DB_FOLDER = "asset_db";
const char* AssetDB::DB_NAME = X_ENGINE_NAME"_asset.db";
const char* AssetDB::RAW_FILES_FOLDER = "raw_files";
const char* AssetDB::THUMBS_FOLDER = "thumbs";

const size_t AssetDB::MAX_COMPRESSOR_SIZE = core::Max<size_t>(
	core::Max(
		core::Max(
			core::Max(
				sizeof(core::Compression::Compressor<core::Compression::LZ4>),
				sizeof(core::Compression::Compressor<core::Compression::LZ4HC>)),
			sizeof(core::Compression::Compressor<core::Compression::LZMA>)),
		sizeof(core::Compression::Compressor<core::Compression::Zlib>)),
	16) + 256;


AssetDB::AssetDB() :
	modId_(INVALID_MOD_ID),
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

	if (!gEnv->pFileSys->fileExists(dbPath.c_str())) {
		X_WARNING("AssetDB", "Failed to find exsisting asset_db creating a new one");
	}


	if (!db_.connect(dbPath.c_str())) {
		return false;
	}

	if (!db_.execute("PRAGMA synchronous = OFF; PRAGMA page_size = 4096; PRAGMA journal_mode=wal; PRAGMA foreign_keys = ON;")) {
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

	open_ = true;
	return true;
}

void AssetDB::CloseDB(void)
{
	open_ = false;
	db_.disconnect();
}

bool AssetDB::CreateTables(void)
{
	if (!db_.execute("CREATE TABLE IF NOT EXISTS thumbs ("
		"thumb_id INTEGER PRIMARY KEY,"
		"width INTEGER NOT NULL,"
		"height INTEGER NOT NULL,"
		"size INTEGER NOT NULL,"
		// do i want to store thumbs with name of hash or asset name? I guess benfit of hash i don't have to know asset name, 
		// and care if asset is renamed.
		"hash TEXT NOT NULL," 
		"lastUpdateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL"
		");")) {
		X_ERROR("AssetDB", "Failed to create 'refs' table");
		return false;
	}

	if (!db_.execute("CREATE TABLE IF NOT EXISTS mods ("
		"mod_id INTEGER PRIMARY KEY,"
		"name TEXT COLLATE NOCASE UNIQUE NOT NULL,"
		"out_dir TEXT NOT NULL"
		");")) {
		X_ERROR("AssetDB", "Failed to create 'mods' table");
		return false;
	}

	if (!db_.execute("CREATE TABLE IF NOT EXISTS file_ids ("
		" file_id INTEGER PRIMARY KEY,"
		"name TEXT COLLATE NOCASE NOT NULL," // names are not unique since we allow same name for diffrent type.
		"type INTEGER NOT NULL,"
		"args TEXT,"
		"argsHash INTEGER,"
		"raw_file INTEGER,"
		"thumb_id INTEGER,"
		"add_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,"
		"lastUpdateTime TIMESTAMP,"
		"parent_id INTEGER NULL,"
		"mod_id INTEGER NOT NULL,"
		"FOREIGN KEY(parent_id) REFERENCES file_ids(file_id),"
		"FOREIGN KEY(mod_id) REFERENCES mods(mod_id),"
		"FOREIGN KEY(thumb_id) REFERENCES thumbs(thumb_id),"
		"unique(name, type)"
		");")) {
		X_ERROR("AssetDB", "Failed to create 'file_ids' table");
		return false;
	}

	if (!db_.execute("CREATE TABLE IF NOT EXISTS raw_files ("
		"file_id INTEGER PRIMARY KEY,"
		"path TEXT NOT NULL,"
		"size INTEGER NOT NULL,"
		"hash INTEGER NOT NULL,"
		"add_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL"
		");")) {
		X_ERROR("AssetDB", "Failed to create 'raw_files' table");
		return false;
	}

	// this table contains a list of assets refrences.
	// so if a material is refrenced by 2 models there we be two entries
	// with a toId matching the id of the material.
	if (!db_.execute("CREATE TABLE IF NOT EXISTS refs ("
		"ref_id INTEGER PRIMARY KEY,"
		"toId INTEGER NOT NULL," 
		"fromId INTEGER NOT NULL,"
		"FOREIGN KEY(ref_id) REFERENCES file_ids(file_id),"
		"FOREIGN KEY(fromId) REFERENCES file_ids(file_id)"
		");")) {
		X_ERROR("AssetDB", "Failed to create 'refs' table");
		return false;
	}

	return true;
}

bool AssetDB::DropTables(void)
{
	if (!db_.execute("DROP TABLE IF EXISTS refs;")) {
		return false;
	}
	if (!db_.execute("DROP TABLE IF EXISTS gdt_files;")) {
		return false;
	}
	if (!db_.execute("DROP TABLE IF EXISTS raw_files;")) {
		return false;
	}
	if (!db_.execute("DROP TABLE IF EXISTS mods;")) {
		return false;
	}
	if (!db_.execute("DROP TABLE IF EXISTS thumbs;")) {
		return false;
	}

	return true;
}

bool AssetDB::AddDefaultMods(void)
{
	core::string core("core");
	core::string base("base");

	if (!ModExsists(core)) {
		AddMod(core, core::Path<char>(R"(C:\Users\WinCat\Documents\code\WinCat\engine\potatoengine\game_folder\mod)"));
	}
	if (!ModExsists(base)) {
		AddMod(base, core::Path<char>(R"(C:\Users\WinCat\Documents\code\WinCat\engine\potatoengine\game_folder\core_assets)"));
	}

	return true;
}

bool AssetDB::AddTestData(size_t numMods, const AssetTypeCountsArr& assetCounts)
{
	// adds a load of junk data for testing.

	core::StackString512 modName("testmod_");
	core::Path<char> outDir("testmod_");

	const uint32_t assetNameLenMin = 4;
	const uint32_t assetNameLenMax = 16;

	for (size_t i = 0; i < numMods; i++) {
		modName.appendFmt("%" PRIuS, i);
		outDir.appendFmt("%" PRIuS "_out", i);

		AddMod(core::string(modName.c_str()), outDir);

		if (!SetMod(core::string(modName.c_str()))) {
			return false;
		}

		sql::SqlLiteTransaction trans(db_);

		for (size_t x = 0; x < AssetType::ENUM_COUNT; x++) 
		{
			// add assets.
			if (assetCounts[x])
			{
				AssetType::Enum type = static_cast<AssetType::Enum>(x);
				for (int32_t j = 0; j < assetCounts[x]; j++)
				{
					core::string name = randomAssetName(assetNameLenMin, assetNameLenMax);

					auto result = AddAsset(trans, type, name);
					while (result == Result::NAME_TAKEN) {
						name = randomAssetName(assetNameLenMin, assetNameLenMax);
						result = AddAsset(trans, type, name);
					}
				}
			}
		}

		trans.commit();
	}
	
	return true;
}


AssetDB::Result::Enum AssetDB::AddMod(const core::string& name, core::Path<char>& outDir)
{
	if (name.isEmpty()) {
		X_ERROR("AssetDB", "Mod with empty name not allowed");
		return Result::ERROR;
	}
	if (!ValidName(name)) {
		X_ERROR("AssetDB", "Mod name \"%s\" has invalid characters", name.c_str());
		return Result::ERROR;
	}

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
	sql::SqlLiteQuery qry(db_, "SELECT mod_id FROM mods WHERE name = ?");
	qry.bind(1, name.c_str());

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

bool AssetDB::SetMod(ModId id)
{
	sql::SqlLiteTransaction trans(db_, true);
	sql::SqlLiteQuery qry(db_, "SELECT name FROM mods WHERE mod_id = ?");
	qry.bind(1, id);

	sql::SqlLiteQuery::iterator it = qry.begin();

	if (it != qry.end())
	{
		const char* pName = (*it).get<const char*>(0);

		modId_ = id;
		X_LOG0("AssetDB", "Mod set: \"%s\" id: %i", pName, modId_);
		return true;
	}

	X_ERROR("AssetDB", "Failed to set mod no mod with id \"%" PRIu32 "\" exsists", id);
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
	qry.bind(1, id);

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


bool AssetDB::GetAssetTypeCounts(ModId modId, AssetTypeCountsArr& countsOut)
{
	for (size_t i = 0; i<AssetType::ENUM_COUNT; i++) {
		countsOut[i] = 0;
	}

	sql::SqlLiteQuery qry(db_, "SELECT type, COUNT(*) FROM file_ids WHERE mod_id = ? GROUP BY type");
	qry.bind(1, modId);

	auto it = qry.begin();
	if (it == qry.end()) {
		return false;
	}

	for (; it != qry.end(); ++it)
	{
		auto row = *it;

		const int32_t type = row.get<int32_t>(0);
		const int32_t count = row.get<int32_t>(1);

		if (type >= AssetType::ENUM_COUNT) {
			X_ERROR("", "Asset type out of range: %s", AssetType::ToString(type));
			return false;
		}

		countsOut[type] = count;
	}

	return true;
}

bool AssetDB::GetAssetTypeCount(ModId modId, AssetType::Enum type, int32_t& countOut)
{
	countOut = -1; // meow

	sql::SqlLiteTransaction trans(db_, true);
	sql::SqlLiteQuery qry(db_, "SELECT COUNT(*) FROM file_ids WHERE mod_id = ? AND TYPE = ?");
	qry.bind(1, modId);
	qry.bind(2, type);

	sql::SqlLiteQuery::iterator it = qry.begin();
	if (it != qry.end())
	{
		const auto row = *it;
		const int32_t count = row.get<int32_t>(0);

		countOut = count;
		return true;
	}

	X_ERROR("AssetDB", "Failed to get asset count for mod: %" PRIu32 " Asset type: %s", modId, AssetType::ToString(type));
	return false;
}


bool AssetDB::GetAssetList(ModId modId, AssetType::Enum type, AssetInfoArr& assetsOut)
{
	int32_t count;
	if (!GetAssetTypeCount(modId, type, count)) {
		return false;
	}

	// how confident are we count always be correct..
	// could resize if so.
	assetsOut.reserve(count);

	sql::SqlLiteQuery qry(db_, "SELECT file_id, parent_id, type, name FROM file_ids WHERE mod_id = ? AND type = ?");
	qry.bind(1, modId);
	qry.bind(2, type);

	auto it = qry.begin();
	for (; it != qry.end(); ++it)
	{
		auto row = *it;

		const int32_t id = row.get<int32_t>(0);
		const AssetType::Enum type = static_cast<AssetType::Enum>(row.get<int32_t>(2));
		const char* pName = row.get<const char*>(3);

		int32_t parentId = INVALID_ASSET_ID;
		if (row.columnType(1) != sql::ColumType::SNULL) {
			parentId = row.get<int32_t>(1);
		}

		assetsOut.emplace_back(id, parentId, pName, type);
	}

	return true;
}

bool AssetDB::GetModsList(ModsArr& arrOut)
{
	sql::SqlLiteQuery qry(db_, "SELECT mod_id, name, out_dir FROM mods");

	auto it = qry.begin();
	for (; it != qry.end(); ++it)
	{
		auto row = *it;

		const int32_t modId = row.get<int32_t>(0);
		const char* pName = row.get<const char*>(1);
		const char* pOutdir = row.get<const char*>(2);

		arrOut.emplace_back(modId, pName, pOutdir);
	}

	return true;
}

bool AssetDB::IterateMods(ModDelegate func)
{
	sql::SqlLiteQuery qry(db_, "SELECT mod_id, name, out_dir FROM mods");

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


bool AssetDB::IterateAssets(AssetDelegate func)
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

bool AssetDB::IterateAssets(ModId modId, AssetDelegate func)
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

bool AssetDB::IterateAssets(AssetType::Enum type, AssetDelegate func)
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


AssetDB::Result::Enum AssetDB::AddAsset(const sql::SqlLiteTransaction& trans, AssetType::Enum type, const core::string& name, int32_t* pId)
{
	if (name.isEmpty()) {
		X_ERROR("AssetDB", "Asset with empty name not allowed");
		return Result::ERROR;
	}
	if (!ValidName(name)) {
		X_ERROR("AssetDB", "Asset name \"%s\" is invalid", name.c_str());
		return Result::ERROR;
	}

	if (!isModSet()) {
		X_ERROR("AssetDB", "Mod must be set before calling AddAsset!");
		return Result::ERROR;
	}

	sql::SqlLiteCmd cmd(db_, "INSERT INTO file_ids (name, type, mod_id) VALUES(?,?,?)");
	cmd.bind(1, name.c_str());
	cmd.bind(2, type);
	cmd.bind(3, modId_);


	sql::Result::Enum res = cmd.execute();
	if (res != sql::Result::OK)
	{
		if (res == sql::Result::CONSTRAINT) {
			return Result::NAME_TAKEN;
		}

		return Result::ERROR;
	}

	if (pId) {
		*pId = safe_static_cast<int32_t, sql::SqlLiteDb::RowId>(db_.lastInsertRowid());
	}

	return Result::OK;
}


AssetDB::Result::Enum AssetDB::AddAsset(AssetType::Enum type, const core::string& name, int32_t* pId)
{
	if (!isModSet()) {
		X_ERROR("AssetDB", "Mod must be set before calling AddAsset!");
		return Result::ERROR;
	}

	return AddAsset(modId_, type, name, pId);
}

AssetDB::Result::Enum AssetDB::AddAsset(ModId modId, AssetType::Enum type, const core::string& name, int32_t* pId)
{
	if (name.isEmpty()) {
		X_ERROR("AssetDB", "Asset with empty name not allowed");
		return Result::ERROR;
	}
	if (!ValidName(name)) {
		X_ERROR("AssetDB", "Asset name \"%s\" is invalid", name.c_str());
		return Result::ERROR;
	}

	sql::SqlLiteTransaction trans(db_);
	sql::SqlLiteCmd cmd(db_, "INSERT INTO file_ids (name, type, mod_id) VALUES(?,?,?)");
	cmd.bind(1, name.c_str());
	cmd.bind(2, type);
	cmd.bind(3, modId);


	sql::Result::Enum res = cmd.execute();
	if (res != sql::Result::OK)
	{
		if (res == sql::Result::CONSTRAINT) {
			return Result::NAME_TAKEN;
		}

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
	int32_t assetId;
	if (AssetExsists(type, name, &assetId)) {
		X_ERROR("AssetDB", "Failed to delete asset: %s:%s it does not exist", AssetType::ToString(type), name.c_str());
		return Result::ERROR;
	}

	// do we have any refs ?
	uint32_t numRefs;
	if (!GetAssetRefCount(assetId, numRefs)) {
		X_ERROR("AssetDB", "Failed to delete asset: %s:%s error getting ref count", AssetType::ToString(type), name.c_str());
		return Result::ERROR;
	}

	if (numRefs > 0) {
		X_ERROR("AssetDB", "Failed to delete asset: %s:%s %" PRIu32 " assets are referencing it", 
			AssetType::ToString(type), name.c_str(), numRefs);
		return Result::HAS_REFS;
	}

	// are we a parent?
	if (AssetIsParent(assetId)) {
		X_ERROR("AssetDB", "Failed to delete asset: %s:%s %" PRIu32 " it has child assets",
			AssetType::ToString(type), name.c_str());
		return Result::HAS_REFS; // technically it has refs by been a parent, so reuse this and who ever gets the error should just work it out.
	}

	// If we are a child, it don't matter currently
	// as the child stores the link, not the parent.

	sql::SqlLiteTransaction trans(db_);
	sql::SqlLiteCmd cmd(db_, "DELETE FROM file_ids WHERE file_id = ? AND type = ? AND name = ?");
	cmd.bind(1, assetId); // make sure it's same one we found above.
	cmd.bind(2, type);
	cmd.bind(3, name.c_str());

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

	if (newName.isEmpty()) {
		X_ERROR("AssetDB", "Can't rename asset \"%s\" to Blank name", newName.c_str());
		return Result::ERROR;
	}

	if (!ValidName(newName)) {
		X_ERROR("AssetDB", "Can't rename asset \"%s\" to \"%s\" new name is invalid", name.c_str(), newName.c_str());
		return Result::ERROR;
	}

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
	const DataArr& data, const core::string& argsOpt)
{
	int32_t assetId, rawId;

	if (name.isEmpty()) {
		X_ERROR("AssetDB", "Can't update asset with empty name");
		return Result::ERROR;
	}

#if X_ENABLE_ASSERTIONS
	assetId = INVALID_ASSET_ID;
#endif // !X_ENABLE_ASSERTIONS

	if (!AssetExsists(type, name, &assetId)) {
		// add it?
		if (AddAsset(type, name, &assetId) != Result::OK) {
			X_ERROR("AssetDB", "Failed to add assert when trying to update a asset that did not exsists.");
			return Result::NOT_FOUND;
		}

		X_WARNING("AssetDB", "Added asset to db as it didnt exists when trying to update the asset");
	}

	X_ASSERT(assetId != INVALID_ASSET_ID, "AssetId is invalid")();

	core::string args(argsOpt);

	if(!MergeArgs(assetId, args)) {
		X_WARNING("AssetDB", "Failed to merge args");
		return Result::ERROR;
	}

	core::Crc32* pCrc32 = gEnv->pCore->GetCrc32();
	const uint32_t dataCrc = pCrc32->GetCRC32(data.ptr(), data.size());
	const uint32_t argsCrc = pCrc32->GetCRC32(args.c_str(), args.length());

	rawId = INVALID_RAWFILE_ID;
	if (data.isNotEmpty())
	{
		RawFile rawData;

		if (GetRawfileForId(assetId, rawData, &rawId))
		{
			if (rawData.hash == dataCrc) 
			{
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


		if (rawId == INVALID_RAWFILE_ID)
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
			cmd.bind(4, rawId);

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


AssetDB::Result::Enum AssetDB::UpdateAssetRawFile(int32_t assetId, const DataArr& data, core::Compression::Algo::Enum algo,
	core::Compression::CompressLevel::Enum lvl)
{
	AssetInfo info;

	if (GetAssetInfoForAsset(assetId, info)) {
		return Result::ERROR;
	}

	return UpdateAssetRawFile(info.type, info.name, data, algo, lvl);
}

AssetDB::Result::Enum AssetDB::UpdateAssetRawFile(AssetType::Enum type, const core::string& name, const DataArr& data,
	core::Compression::Algo::Enum algo, core::Compression::CompressLevel::Enum lvl)
{
	// compress it.
	X_ALIGNED_SYMBOL(char buf[MAX_COMPRESSOR_SIZE], 16);
	core::LinearAllocator allocator(buf, buf + sizeof(buf));

	auto* pCompressor = AllocCompressor(&allocator, algo);

	core::StopWatch timer;

	DataArr compressed(data.getArena());
	if (!pCompressor->deflate(data.getArena(), data, compressed, lvl))
	{
		X_ERROR("AssetDB", "Failed to defalte raw file data");
		return Result::ERROR;
	}
	else
	{
		const auto elapsed = timer.GetMilliSeconds();
		const float percentageSize = (static_cast<float>(compressed.size()) / static_cast<float>(data.size())) * 100;

		core::HumanSize::Str sizeStr, sizeStr2;
		X_LOG2("AssetDB", "Defalated raw file %s -> %s(%.2g%%) %gms",
			core::HumanSize::toString(sizeStr, data.size()),
			core::HumanSize::toString(sizeStr2, compressed.size()),
			percentageSize,
			elapsed);
	}

	core::Mem::Destruct(pCompressor);

	return UpdateAssetRawFile(type, name, compressed);
}


AssetDB::Result::Enum AssetDB::UpdateAssetRawFile(int32_t assetId, const DataArr& data)
{
	// we make use of the asset name and type, so the main logic is in that one.
	AssetInfo info;

	if (GetAssetInfoForAsset(assetId, info)) {
		return Result::ERROR;
	}

	return UpdateAssetRawFile(info.type, info.name, data);
}

AssetDB::Result::Enum AssetDB::UpdateAssetRawFile(AssetType::Enum type, const core::string& name,
	const DataArr& data)
{
	if (name.isEmpty()) {
		X_ERROR("AssetDB", "Can't update asset with empty name");
		return Result::ERROR;
	}

	int32_t assetId;

#if X_ENABLE_ASSERTIONS
	assetId = INVALID_ASSET_ID;
#endif // !X_ENABLE_ASSERTIONS

	if (!AssetExsists(type, name, &assetId)) {
		// add it?
		if (AddAsset(type, name, &assetId) != Result::OK) {
			X_ERROR("AssetDB", "Failed to add assert when trying to update a asset that did not exsists.");
			return Result::NOT_FOUND;
		}

		X_WARNING("AssetDB", "Added asset to db as it didnt exists when trying to update the asset");
	}

	X_ASSERT(assetId != INVALID_ASSET_ID, "AssetId is invalid")();

	core::Crc32* pCrc32 = gEnv->pCore->GetCrc32();
	const uint32_t dataCrc = pCrc32->GetCRC32(data.ptr(), data.size());

	int32_t rawId = INVALID_RAWFILE_ID;
	if (data.isNotEmpty())
	{
		RawFile rawData;

		if (GetRawfileForId(assetId, rawData, &rawId))
		{
			if (rawData.hash == dataCrc)
			{
				X_LOG0("AssetDB", "Skipping raw file update file unchanged");
				return Result::UNCHANGED;
			}
		}
	}

	// start the transaction.
	sql::SqlLiteTransaction trans(db_);

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

			// path include folder, so don't need type to load it.
			// We need this in addition to AssetPathForName
			// so that the raw_files path works without needing asset type
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

		if (rawId == INVALID_RAWFILE_ID)
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
			cmd.bind(4, rawId);

			sql::Result::Enum res = cmd.execute();
			if (res != sql::Result::OK) {
				return Result::ERROR;
			}
		}
	}

	trans.commit();
	return Result::OK;
}

AssetDB::Result::Enum AssetDB::UpdateAssetArgs(AssetType::Enum type, const core::string& name, const core::string& argsOpt)
{
	int32_t assetId;

	if (name.isEmpty()) {
		X_ERROR("AssetDB", "Can't update asset args with empty name");
		return Result::ERROR;
	}

	if (!AssetExsists(type, name, &assetId))  {
		// I don't allow adding of asset if not providing raw data also.
		X_WARNING("AssetDB", "Failed to update asset args, asset not found");
		return Result::NOT_FOUND;
	}

	core::string args(argsOpt);

	if (!MergeArgs(assetId, args)) {
		X_WARNING("AssetDB", "Failed to merge args");
		return Result::ERROR;
	}

	core::Crc32* pCrc32 = gEnv->pCore->GetCrc32();
	const uint32_t argsCrc = pCrc32->GetCRC32(args.c_str(), args.length());

	sql::SqlLiteTransaction trans(db_);
	core::string stmt;

	stmt = "UPDATE file_ids SET lastUpdateTime = DateTime('now'), args = :args, argsHash = :argsHash"
		" WHERE type = :t AND name = :n ";

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
 
AssetDB::Result::Enum AssetDB::UpdateAssetThumb(AssetType::Enum type, const core::string& name, Vec2i dimensions, const DataArr& data)
{
	int32_t assetId;

	if (!AssetExsists(type, name, &assetId)) {
		return Result::NOT_FOUND;
	}

	return UpdateAssetThumb(assetId, dimensions, data);
}

AssetDB::Result::Enum AssetDB::UpdateAssetThumb(int32_t assetId, Vec2i dimensions, const DataArr& data)
{
	// so my little floating goat, we gonna store the thumbs with hash names.
	// that way i don't need to rename the fuckers if i rename the asset.
	// might do same for raw assets at somepoint...

	core::Hash::MD5 hasher;
	hasher.update(data.data(), data.size());
	const auto hash = hasher.finalize();

	int32_t thumbId = INVALID_THUMB_ID;
	if (data.isNotEmpty())
	{
		ThumbInfo thumb;

		if (GetThumbInfoForId(assetId, thumb, &thumbId))
		{
			if (thumb.hash == hash)
			{
				X_LOG0("AssetDB", "Skipping thumb update file unchanged");
				return Result::UNCHANGED;
			}
		}
	}


	core::StopWatch timer;

	// compress the thumb with a quick pass.
	DataArr compressed(data.getArena());
	core::Compression::Compressor<core::Compression::LZ4> comp;

	if (!comp.deflate(data.getArena(), data, compressed, core::Compression::CompressLevel::NORMAL))
	{
		X_ERROR("AssetDB", "Failed to defalte thumb data");
		return Result::ERROR;
	}
	else
	{
		const auto elapsed = timer.GetMilliSeconds();
		const float percentageSize = (static_cast<float>(compressed.size()) / static_cast<float>(data.size())) * 100;

		core::HumanSize::Str sizeStr, sizeStr2;
		X_LOG2("AssetDB", "Defalated thumb %s -> %s(%.2g%%) %gms",
			core::HumanSize::toString(sizeStr, data.size()),
			core::HumanSize::toString(sizeStr2, compressed.size()),
			percentageSize,
			elapsed);
	}

	sql::SqlLiteTransaction trans(db_);


	// write new data.
	{
		core::Path<char> filePath;
		core::Hash::MD5Digest::String strBuf;

		filePath = ASSET_DB_FOLDER;
		filePath.ensureSlash();
		filePath /= THUMBS_FOLDER;
		filePath.toLower();
		filePath.ensureSlash();
		filePath /= hash.ToString(strBuf);

		// if a thumb with same md5 exsists don't update.
		// now we wait for a collsion, (that we notice) before this code needs updating :D
		if (!gEnv->pFileSys->fileExists(filePath.c_str()))
		{
			if (!gEnv->pFileSys->createDirectoryTree(filePath.c_str())) {
				X_ERROR("AssetDB", "Failed to create dir to save thumb");
				return Result::ERROR;
			}

			core::fileModeFlags mode;
			mode.Set(core::fileMode::WRITE);
			mode.Set(core::fileMode::SHARE);

			core::XFileScoped file;

			if (!file.openFile(filePath.c_str(), mode)) {
				X_ERROR("AssetDB", "Failed to write thumb");
				return Result::ERROR;
			}

			if (file.write(compressed.ptr(), compressed.size()) != compressed.size()) {
				X_ERROR("AssetDB", "Failed to write thumb data");
				return Result::ERROR;
			}
		}
	}

	// so we want to insert or update.

	if (thumbId == INVALID_THUMB_ID)
	{
		sql::SqlLiteDb::RowId lastRowId;

		// insert.
		{
			sql::SqlLiteCmd cmd(db_, "INSERT INTO thumbs (width, height, size, hash) VALUES(?,?,?,?)");
			cmd.bind(1, dimensions.x);
			cmd.bind(2, dimensions.y);
			cmd.bind(3, safe_static_cast<int32_t, size_t>(compressed.size()));
			cmd.bind(4, &hash, sizeof(hash));

			sql::Result::Enum res = cmd.execute();
			if (res != sql::Result::OK) {
				return Result::ERROR;
			}

			lastRowId = db_.lastInsertRowid();
		}

		// set thumb id..
		{
			sql::SqlLiteCmd cmd(db_, "UPDATE file_ids SET thumb_id = ? WHERE file_id = ?");
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
		sql::SqlLiteCmd cmd(db_, "UPDATE thumbs SET width = ?, height = ?, size = ?, hash = ?, lastUpdateTime = DateTime('now') WHERE thumb_id = ?");
		cmd.bind(1, dimensions.x);
		cmd.bind(2, dimensions.y);
		cmd.bind(3, safe_static_cast<int32_t, size_t>(compressed.size()));
		cmd.bind(4, &hash, sizeof(hash));
		cmd.bind(5, thumbId);

		sql::Result::Enum res = cmd.execute();
		if (res != sql::Result::OK) {
			return Result::ERROR;
		}
	}

	trans.commit();
	return Result::OK;
}


bool AssetDB::AssetExsists(AssetType::Enum type, const core::string& name, int32_t* pIdOut, ModId* pModIdOut)
{
	if (name.isEmpty()) {
		return false;
	}

	sql::SqlLiteQuery qry(db_, "SELECT file_id, mod_id FROM file_ids WHERE type = ? AND name = ?");
	qry.bind(1, type);
	qry.bind(2, name.c_str());

	const auto it = qry.begin();

	if (it == qry.end()) {
		return false;
	}

	if (pIdOut){
		*pIdOut = (*it).get<int32_t>(0);
	}
	if (pModIdOut) {
		*pModIdOut = (*it).get<int32_t>(1);
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

	// args can be null.
	if ((*it).columnType(0) != sql::ColumType::SNULL) {
		argsOut = (*it).get<const char*>(0);
	}
	else {
		argsOut.clear();

		// should we return valid json?
		argsOut = "{}";
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

	// args can be null.
	if ((*it).columnType(0) != sql::ColumType::SNULL) {
		argsHashOut = static_cast<uint32_t>((*it).get<int32_t>(0));
	}
	else {
		argsHashOut = 0;
	}
	return true;
}


bool AssetDB::GetModIdForAsset(int32_t assetId, ModId& modIdOut)
{
	sql::SqlLiteQuery qry(db_, "SELECT mod_id FROM file_ids WHERE file_ids.file_id = ?");
	qry.bind(1, assetId);

	const auto it = qry.begin();

	if (it == qry.end()) {
		return false;
	}

	modIdOut = static_cast<ModId>((*it).get<int32_t>(0));
	return true;
}

bool AssetDB::GetRawFileDataForAsset(int32_t assetId, DataArr& dataOut)
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

	const size_t size = safe_static_cast<size_t, uint64_t>(file.remainingBytes());

	DataArr compressedData(g_AssetDBArena);
	compressedData.resize(size);

	if (file.read(compressedData.ptr(), size) != size) {
		X_ERROR("AssetDB", "Failed to read rawfile contents");
		return false;
	}

	// decompress it.
	const Algo::Enum algo = ICompressor::getAlgo(compressedData);

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

bool AssetDB::GetThumbForAsset(int32_t assetId, ThumbInfo& info, DataArr& thumbDataOut)
{
	using namespace core::Compression;

	if (!GetThumbInfoForId(assetId, info)) {
	// just treat it as no thumb.
	//	X_ERROR("AssetDB", "Failed to get thumb info for data reterival");
		return false;
	}

	// load the file.
	core::XFileScoped file;
	core::fileModeFlags mode;
	core::Path<char> filePath;

	mode.Set(core::fileMode::READ);
	mode.Set(core::fileMode::SHARE);

	ThumbPathForThumb(info, filePath);

	if (!file.openFile(filePath.c_str(), mode)) {
		X_ERROR("AssetDB", "Failed to open thumb");
		return false;
	}

	const size_t size = info.fileSize;

	DataArr compressedData(g_AssetDBArena);
	compressedData.resize(size);

	if (file.read(compressedData.ptr(), size) != size) {
		X_ERROR("AssetDB", "Failed to read thumb contents");
		return false;
	}

	// decompress it.
	Algo::Enum algo = ICompressor::getAlgo(compressedData);

	if (algo == Algo::LZ4) {
		Compressor<LZ4> def;
		return def.inflate(g_AssetDBArena, compressedData, thumbDataOut);
	}
	else if (algo == Algo::LZMA) {
		Compressor<LZMA> def;
		return def.inflate(g_AssetDBArena, compressedData, thumbDataOut);
	}
	else if (algo == Algo::ZLIB) {
		Compressor<Zlib> def;
		return def.inflate(g_AssetDBArena, compressedData, thumbDataOut);
	}

	X_ERROR("AssetDB", "Failed to read thumb, unkown compression algo");
	return false;



}


bool AssetDB::GetTypeForAsset(int32_t assetId, AssetType::Enum& typeOut)
{
	sql::SqlLiteQuery qry(db_, "SELECT type FROM file_ids WHERE file_ids.file_id = ?");
	qry.bind(1, assetId);

	const auto it = qry.begin();

	if (it == qry.end()) {
		return false;
	}

	typeOut = static_cast<AssetType::Enum>((*it).get<int32_t>(0));
	return true;
}

bool AssetDB::GetAssetInfoForAsset(int32_t assetId, AssetInfo& infoOut)
{
	sql::SqlLiteQuery qry(db_, "SELECT name, file_id, parent_id, type FROM file_ids WHERE file_ids.file_id = ?");
	qry.bind(1, assetId);

	const auto it = qry.begin();

	if (it == qry.end()) {
		infoOut = AssetInfo();
		return false;
	}

	infoOut.name = (*it).get<const char*>(0);
	infoOut.id = (*it).get<int32_t>(1);
	infoOut.parentId = (*it).get<int32_t>(2);
	infoOut.type = static_cast<AssetType::Enum>((*it).get<int32_t>(3));
	return true;
}


bool AssetDB::GetAssetRefCount(int32_t assetId, uint32_t& refCountOut)
{
	sql::SqlLiteQuery qry(db_, "SELECT COUNT(*) from refs WHERE toId = ?");
	qry.bind(1, assetId);

	auto it = qry.begin();
	if (it != qry.end())
	{
		auto row = *it;

		const int32_t count = row.get<int32_t>(0);

		refCountOut = count;
		return true;
	}

	refCountOut = 0;
	return true;
}

bool AssetDB::IterateAssetRefs(int32_t assetId, core::Delegate<bool(int32_t)> func)
{
	sql::SqlLiteQuery qry(db_, "SELECT fromId from refs WHERE toId = ?");
	qry.bind(1, assetId);

	auto it = qry.begin();
	for (; it != qry.end(); ++it)
	{
		auto row = *it;

		const int32_t refId = row.get<int32_t>(0);

		func.Invoke(refId);
	}

	return true;
}


bool AssetDB::GetAssetRefs(int32_t assetId, AssetIdArr& refsOut)
{
	refsOut.clear();

	sql::SqlLiteQuery qry(db_, "SELECT fromId from refs WHERE toId = ?");
	qry.bind(1, assetId);

	auto it = qry.begin();
	for (; it != qry.end(); ++it)
	{
		auto row = *it;

		const int32_t refId = row.get<int32_t>(0);

		refsOut.emplace_back(refId);
	}

	return true;
}


AssetDB::Result::Enum AssetDB::AddAssertRef(int32_t assetId, int32_t targetAssetId)
{
	// check if we already have a ref.
	{
		sql::SqlLiteQuery qry(db_, "SELECT ref_id from refs WHERE toId = ? AND fromId = ?");
		qry.bind(1, targetAssetId);
		qry.bind(2, assetId);

		if (qry.begin() != qry.end())
		{
			X_ERROR("AssetDB", "Failed to add assert ref %" PRIi32 " -> %" PRIi32 " as a ref already exists", assetId, targetAssetId);
			return Result::ERROR;
		}
	}

	// check for circular ref
	{
		sql::SqlLiteQuery qry(db_, "SELECT ref_id from refs WHERE toId = ? AND fromId = ?");
		qry.bind(1, assetId);
		qry.bind(2, targetAssetId);

		if (qry.begin() != qry.end())
		{
			X_ERROR("AssetDB", "Failed to add assert ref %" PRIi32 " -> %" PRIi32 " would create circular dep", assetId, targetAssetId );
			return Result::ERROR;
		}
	}

	sql::SqlLiteTransaction trans(db_);
	sql::SqlLiteCmd cmd(db_, "INSERT INTO refs (fromId, toId) VALUES(?,?)");
	cmd.bind(1, assetId);
	cmd.bind(2, targetAssetId);

	sql::Result::Enum res = cmd.execute();
	if (res != sql::Result::OK) {
		return Result::ERROR;
	}

	trans.commit();
	return Result::OK;
}


AssetDB::Result::Enum AssetDB::RemoveAssertRef(int32_t assetId, int32_t targetAssetId)
{
	sql::SqlLiteTransaction trans(db_);
	sql::SqlLiteCmd cmd(db_, "DELETE FROM refs WHERE fromId = ? AND toId = ?");
	cmd.bind(1, assetId);
	cmd.bind(2, targetAssetId);

	sql::Result::Enum res = cmd.execute();
	if (res != sql::Result::OK) {
		return Result::ERROR;
	}

	trans.commit();
	return Result::OK;
}

bool AssetDB::AssetHasParent(int32_t assetId, int32_t* pParentId)
{
	sql::SqlLiteQuery qry(db_, "SELECT parent_id FROM file_ids WHERE file_ids.file_id = ?");
	qry.bind(1, assetId);

	const auto it = qry.begin();

	if (it == qry.end()) {
		// humm should not happen, you given me bad asset id!
		X_ERROR("AssetDB", "Failed to get asset info for id: %" PRIi32, assetId);
		return false;
	}

	if (pParentId) { // null check for the plebs
		if ((*it).columnType(0) != sql::ColumType::SNULL) {
			*pParentId = static_cast<int32_t>((*it).get<int32_t>(0));

		}
		else {
			*pParentId = INVALID_ASSET_ID;
		}
	}

	return true;
}

bool AssetDB::AssetIsParent(int32_t assetId)
{
	sql::SqlLiteQuery qry(db_, "SELECT file_id FROM file_ids WHERE file_ids.parent_id = ?");
	qry.bind(1, assetId);

	if (qry.begin() == qry.end()) {
		return false;
	}

	return true;
}


AssetDB::Result::Enum AssetDB::SetAssetParent(int32_t assetId, int32_t parentAssetId)
{
	// we don't allow parent updating.
	// you must explicity remove parent then add new to change.
	if (AssetHasParent(assetId)) {
		X_ERROR("AssetDB", "Failed to set asset %" PRIi32 " parent, it already has a parent", assetId);
		return Result::ERROR;
	}

	{
		AssetType::Enum type, parentType;
		if (GetTypeForAsset(assetId, type)) {
			X_ERROR("AssetDB", "Failed to get asset %" PRIi32 " type", assetId);
			return Result::ERROR;
		}
		if (GetTypeForAsset(parentAssetId, parentType)) {
			X_ERROR("AssetDB", "Failed to get asset %" PRIi32 " type", parentAssetId);
			return Result::ERROR;
		}

		if (type != parentType) {
			X_ERROR("AssetDB", "Failed to set asset %" PRIi32 " parent, the parent is a diffrent asset type: %s -> %s",
				assetId, AssetType::ToString(type), AssetType::ToString(parentType));
			return Result::ERROR;
		}
	}

	sql::SqlLiteTransaction trans(db_);
	sql::SqlLiteCmd cmd(db_, "UPDATE file_ids SET parent_id = ? WHERE file_ids.file_id = ?");
	cmd.bind(1, parentAssetId);
	cmd.bind(2, assetId);

	sql::Result::Enum res = cmd.execute();
	if (res != sql::Result::OK) {
		return Result::ERROR;
	}

	trans.commit();
	return Result::OK;
}

AssetDB::Result::Enum AssetDB::RemoveAssetParent(int32_t assetId)
{
	sql::SqlLiteTransaction trans(db_);

	sql::SqlLiteCmd cmd(db_, "UPDATE file_ids SET parent_id = NULL WHERE file_ids.file_id = ?");
	cmd.bind(1, assetId);

	sql::Result::Enum res = cmd.execute();
	if (res != sql::Result::OK) {
		return Result::ERROR;
	}

	trans.commit();
	return Result::OK;
}

bool AssetDB::GetRawfileForId(int32_t assetId, RawFile& dataOut, int32_t* pRawFileId)
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

	if (pRawFileId) {
		*pRawFileId = (*it).get<int32_t>(0);
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

bool AssetDB::GetThumbInfoForId(int32_t assetId, ThumbInfo& dataOut, int32_t* pThumbId)
{
	sql::SqlLiteQuery qry(db_, "SELECT thumbs.thumb_id, thumbs.width, thumbs.height, thumbs.size, thumbs.hash FROM thumbs "
	"INNER JOIN file_ids on thumbs.thumb_id = file_ids.thumb_id WHERE file_ids.file_id = ?");
	qry.bind(1, assetId);

	const auto it = qry.begin();

	if (it == qry.end()) {
		return false;
	}


	dataOut.id = (*it).get<int32_t>(0);
	dataOut.dimension.x = (*it).get<int32_t>(1);
	dataOut.dimension.y = (*it).get<int32_t>(2);
	dataOut.fileSize = (*it).get<int32_t>(3);

	if (pThumbId) {
		*pThumbId = dataOut.id;
	}

	const void* pHash = (*it).get<void const*>(4);
	const size_t hashBlobSize = (*it).columnBytes(4);
	// u fucking pineapple.
	if (hashBlobSize != sizeof(dataOut.hash.bytes)) {
		X_ERROR("AssetDB", "THumb hash blob incorrect size: %" PRIuS, hashBlobSize);
		return false;
	}

	std::memcpy(dataOut.hash.bytes, pHash, sizeof(dataOut.hash.bytes));
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

void AssetDB::ThumbPathForThumb(const ThumbInfo& thumb, core::Path<char>& pathOut)
{
	core::Hash::MD5Digest::String hashStr;

	pathOut = ASSET_DB_FOLDER;
	pathOut.ensureSlash();
	pathOut /= THUMBS_FOLDER;
	pathOut.ensureSlash();
	pathOut /= thumb.hash.ToString(hashStr);
}


bool AssetDB::ValidName(const core::string& name)
{
	if (name.length() > ASSET_NAME_MAX_LENGTH) {
		X_ERROR("AssetDB", "Asset name \"%s\" with length %" PRIuS " exceeds max lenght of %" PRIuS, 
			name.c_str(), name.length(), ASSET_NAME_MAX_LENGTH);
		return false;
	}

	if (name.length() < ASSET_NAME_MIN_LENGTH) {
		X_ERROR("AssetDB", "Asset name \"%s\" with length %" PRIuS " is shorter than min lenght of %" PRIuS,
			name.c_str(), name.length(), ASSET_NAME_MIN_LENGTH);
		return false;
	}


	for (size_t i = 0; i < name.length(); i++)
	{
		// are you valid!?
		const char ch = name[i];

		bool valid = core::strUtil::IsAlphaNum(ch) || core::strUtil::IsDigit(ch) || ch == '_' || ch == ASSET_NAME_SLASH;
		if (!valid) {
			X_ERROR("AssetDB", "Asset name \"%s\" has invalid character at position %" PRIuS, name.c_str(), i + 1);
			return false;
		}

		// provide more info when it's case error.
		if (core::strUtil::IsAlpha(ch) && !core::strUtil::IsLower(ch)) {
			X_ERROR("AssetDB", "Asset name \"%s\" has invalid upper-case character at position %" PRIuS, name.c_str(), 
				i + 1); // make it none 0 index based for the plebs.
			return false;
		}
	}

	return true;
}

core::Compression::ICompressor* AssetDB::AllocCompressor(core::LinearAllocator* pAllocator, core::Compression::Algo::Enum algo)
{
	core::Compression::ICompressor* pCom = nullptr;

	static_assert(core::Compression::Algo::ENUM_COUNT == 4, "Added additional compression algos? this code needs updating.");

	switch (algo)
	{
	case core::Compression::Algo::LZ4:
		pCom = core::Mem::Construct<core::Compression::Compressor<core::Compression::LZ4>>(pAllocator->allocate(sizeof(core::Compression::Compressor<core::Compression::LZ4>), X_ALIGN_OF(core::Compression::Compressor<core::Compression::LZ4>), 0));
		break;
	case core::Compression::Algo::LZ4HC:
		pCom = core::Mem::Construct<core::Compression::Compressor<core::Compression::LZ4>>(pAllocator->allocate(sizeof(core::Compression::Compressor<core::Compression::LZ4>), X_ALIGN_OF(core::Compression::Compressor<core::Compression::LZ4>), 0));
		break;
	case core::Compression::Algo::LZMA:
		pCom = core::Mem::Construct<core::Compression::Compressor<core::Compression::LZ4>>(pAllocator->allocate(sizeof(core::Compression::Compressor<core::Compression::LZ4>), X_ALIGN_OF(core::Compression::Compressor<core::Compression::LZ4>), 0));
		break;
	case core::Compression::Algo::ZLIB:
		pCom = core::Mem::Construct<core::Compression::Compressor<core::Compression::LZ4>>(pAllocator->allocate(sizeof(core::Compression::Compressor<core::Compression::LZ4>), X_ALIGN_OF(core::Compression::Compressor<core::Compression::LZ4>), 0));
		break;

	default:
		X_ASSERT_UNREACHABLE();
		break;
	}

	return pCom;
}


X_NAMESPACE_END