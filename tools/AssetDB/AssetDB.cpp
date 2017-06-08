#include "stdafx.h"
#include "AssetDB.h"

#include <Containers\Array.h>
#include <Hashing\crc32.h>
#include <Hashing\MD5.h>

#include <IFileSys.h>
#include <ICompression.h>

#include <Compression\LZ4.h>
#include <Compression\LZ5.h>
#include <Compression\Lzma2.h>
#include <Compression\Zlib.h>
#include <Compression\Store.h>

#include <String\Json.h>
#include <String\HumanSize.h>
#include <String\StringTokenizer.h>

#include <Random\MultiplyWithCarry.h>

#include <Platform\MessageBox.h>

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

		core::random::MultiplyWithCarry rand;

		const size_t charSetNum = sizeof(charSet) - 1;
		const size_t len = rand.randRange(assetNameLenMin, assetNameLenMax);

		core::string name;
		name.reserve(len);

		bool addSlash = (rand.rand() % 10) == 5;

		for (size_t i = 0; i < len; i++)
		{
			if (addSlash && i < (len - 2) && name.length() > 2 && (rand.rand() % 5) == 1 &&
				name[i-1] != ASSET_NAME_SLASH) {
				name += ASSET_NAME_SLASH;
			}
			else {
				name += charSet[rand.rand() % charSetNum];
			}
		}

		return name;
	}


	// the raw files store compression headers with the algo's in, so verify not changed.
	
	// the count is not importants, and can change just flag that checks should be added for the new algo enum.
	static_assert(core::Compression::Algo::ENUM_COUNT == 7, "More algo's added? add a additional assert here.");

	static_assert(core::Compression::Algo::STORE == 0, "Compression algo index changed");
	static_assert(core::Compression::Algo::LZ4 == 1, "Compression algo index changed");
	static_assert(core::Compression::Algo::LZ4HC == 2, "Compression algo index changed");
	static_assert(core::Compression::Algo::LZMA == 3, "Compression algo index changed");
	static_assert(core::Compression::Algo::ZLIB == 4, "Compression algo index changed");
	static_assert(core::Compression::Algo::LZ5 == 5, "Compression algo index changed");
	static_assert(core::Compression::Algo::LZ5HC == 6, "Compression algo index changed");

	// make sure not changed either.
	X_ENSURE_SIZE(core::Compression::BufferHdr, 16);


} // namespace



// -----------------------------------------------------


const char* AssetDB::ASSET_DB_FOLDER = "asset_db";
const char* AssetDB::DB_NAME = X_ENGINE_NAME"_asset.db";
const char* AssetDB::RAW_FILES_FOLDER = "raw_files";
const char* AssetDB::THUMBS_FOLDER = "thumbs";

static_assert(core::Compression::Algo::ENUM_COUNT == 7, "Added additional compression algos? this code needs updating.");

const size_t AssetDB::MAX_COMPRESSOR_SIZE = core::Max<size_t>(
	core::Max(
		core::Max(
			core::Max(
				core::Max(
					core::Max(
						core::Max(
							sizeof(core::Compression::Compressor<core::Compression::LZ4>),
							sizeof(core::Compression::Compressor<core::Compression::LZ4HC>)),
						sizeof(core::Compression::Compressor<core::Compression::LZ5>)),
					sizeof(core::Compression::Compressor<core::Compression::LZ5HC>)),
				sizeof(core::Compression::Compressor<core::Compression::LZMA>)),
			sizeof(core::Compression::Compressor<core::Compression::Zlib>)),
		sizeof(core::Compression::Compressor<core::Compression::Store>)),
	16) + 256;


AssetDB::AssetDB() :
	modId_(INVALID_MOD_ID),
	dbVersion_(-1),
	open_(false)
{

}

AssetDB::~AssetDB()
{

}


bool AssetDB::OpenDB(ThreadMode::Enum threadMode)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pFileSys);

	if (open_) {
		return true;
	}

	core::Path<char> dbPath;
	dbPath.append(ASSET_DB_FOLDER);
	dbPath.ensureSlash();

	if (!gEnv->pFileSys->createDirectoryTree(dbPath.c_str())) {
		X_ERROR("AssetDB", "Failed to create dir for asset_db");
		return false;
	}

	dbPath.append(DB_NAME);

	const bool dbExsists = gEnv->pFileSys->fileExists(dbPath.c_str());

	if (!dbExsists) {
		X_WARNING("AssetDB", "Failed to find exsisting asset_db creating a new one");
	}

	// I need multi threaded mode when asset db is inside AssetManager
	// as the conversion needs to be run in a background thread.
	// and some actions are done on the UI thread.
	if (!db_.connect(dbPath.c_str(), threadMode)) {
		return false;
	}

	if (dbExsists)
	{
		if (!getDBVersion(dbVersion_)) {
			return false;
		}

		if (dbVersion_ != DB_VERSION) {

			if (dbVersion_ > DB_VERSION) {
				X_ERROR("AssetDB", "DB was made by a newer version of the tool. db version: %" PRIi32 " supported: " PRIi32,
					dbVersion_, DB_VERSION);
				return false;
			}

			if (!PerformMigrations()) {
				X_ERROR("AssetDB", "Failed to perform DB migrations");
				return false;
			}

			if (!setDBVersion(DB_VERSION)) {
				X_ERROR("AssetDB", "Failed to set new DB version to %" PRIi32 " it's recommend you do it manually as migrations passed", DB_VERSION);
				return false;
			}
		}
	}
	else
	{
		// new db set the version.
		if (!setDBVersion(DB_VERSION)) {
			X_ERROR("AssetDB", "Failed to set new DB version to %" PRIi32 " it's recommend you do it manually as migrations passed", DB_VERSION);
			return false;
		}
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

	if (!AddDefaultProfiles()) {
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
		"srcWidth INTEGER,"
		"srcheight INTEGER,"
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
		"compiledHash INTEGER," // the merged hash of the args and rawFile last successfully compiled with.
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
		"FOREIGN KEY(toId) REFERENCES file_ids(file_id),"
		"FOREIGN KEY(fromId) REFERENCES file_ids(file_id)"
		");")) {
		X_ERROR("AssetDB", "Failed to create 'refs' table");
		return false;
	}

	if (!db_.execute("CREATE TABLE IF NOT EXISTS conversion_profiles ("
		"profile_id INTEGER PRIMARY KEY,"
		"name TEXT COLLATE NOCASE NOT NULL,"
		"data TEXT NOT NULL DEFAULT '{}'"
		");")) {
		X_ERROR("AssetDB", "Failed to create 'conversion_profiles' table");
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
	if (!db_.execute("DROP TABLE IF EXISTS conversion_profiles;")) {
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
		AddMod(base, core::Path<char>("base"));
	}

	return true;
}


bool AssetDB::AddDefaultProfiles(void)
{
	core::string dev("dev");
	core::string release("release");

	if (!ProfileExsists(dev)) {
		AddProfile(dev, core::string(R"(
{
    "img": {
        "qualityProfile":"UltraFast"
    }
}
)"));
	}

	if (!ProfileExsists(release)) {
		AddProfile(release, core::string(R"(
{
    "img": {
        "qualityProfile":"Slow"
    }
}
)"));
	}

	return true;
}

bool AssetDB::PerformMigrations(void)
{
	if (dbVersion_ == DB_VERSION) {
		return true;
	}

	core::StackString<1024, char> msg;
	msg.appendFmt("Performing DB migrations from %" PRIi32 " -> %" PRIi32 ". it's recommended to back up the ", dbVersion_, DB_VERSION);
	msg.appendFmt("'%s' folder before pressin Ok. Cancel will skip.", ASSET_DB_FOLDER);

	const auto res = core::msgbox::show(msg.c_str(), "AssetDB Migratation", core::msgbox::Buttons::OKCancel);
	if (res == core::msgbox::Selection::Cancel)
	{
		return false;
	}

	if (dbVersion_ < 1)
	{
		X_WARNING("AssetDB", "Performing migrations from db version %" PRIi32 " to verison 1", dbVersion_);
		// perform upgrades from 0 to 1.
		// must iterate all raw files and rename on disk.
		// there is no DB updates required.

		// could select everything in this query but GetRawfileForId is neater.
		sql::SqlLiteQuery qry(db_, "SELECT file_id FROM raw_files");

		auto it = qry.begin();
		for (; it != qry.end(); ++it)
		{
			auto row = *it;

			int32_t rawFileId = row.get<int32_t>(0);
			RawFile rawfileInfo;

			if (GetRawfileForRawId(rawFileId, rawfileInfo))
			{
				core::Path<char> newFilePath, oldFilePath;
				AssetPathForRawFile(rawfileInfo, newFilePath);

				// build the old path.
				core::StackString512 hashLenStr;
				hashLenStr.appendFmt(".%" PRIu32, rawfileInfo.hash);

				// build old path by removing hash from name.
				oldFilePath.set(newFilePath.begin(), newFilePath.end() - hashLenStr.length());

				X_LOG1("AssetDB", "Renaming raw_file %" PRIi32 " from \"%s\" to \"%s\"", rawFileId, oldFilePath.c_str(), newFilePath.c_str());

				// make sure dir tree for new name is valid.
				// not really needed here since renaming in same folder but it's not hurting anything.
				if (!gEnv->pFileSys->createDirectoryTree(newFilePath.c_str())) {
					X_ERROR("AssetDB", "Failed to create dir to move raw asset");
					// don't early out migrations keep trying the others.
				}

				// if this fails, we return and the update is not commited.
				if (!gEnv->pFileSys->moveFile(oldFilePath.c_str(), newFilePath.c_str())) {
					X_ERROR("AssetDB", "Failed to move asset raw file");
				}
			}
		}
	}

	if (dbVersion_ < 2)
	{
		X_WARNING("AssetDB", "Performing migrations from db version %" PRIi32 " to verison 2", dbVersion_);

		using core::Compression::Algo;

		typedef std::pair<uint8_t, Algo::Enum> AlgoEnumPair;

		// maps from old to new.
		const std::array<AlgoEnumPair, Algo::ENUM_COUNT> algoMap = {{
			{ 4, Algo::STORE }, // store was 4
			{ 0, Algo::LZ4 },
			{ 1, Algo::LZ4HC },
			{ 2, Algo::LZMA },
			{ 3, Algo::ZLIB },
			// unchanged
			{ Algo::LZ5, Algo::LZ5 },
			{ Algo::LZ5HC, Algo::LZ5HC },
		}};


		sql::SqlLiteQuery qry(db_, "SELECT file_id FROM raw_files");

		auto it = qry.begin();
		for (; it != qry.end(); ++it)
		{
			auto row = *it;

			int32_t rawFileId = row.get<int32_t>(0);
			RawFile rawfileInfo;

			if (!GetRawfileForRawId(rawFileId, rawfileInfo))
			{
				X_ERROR("AssetDB", "Failed to get rawfile path");
				return false;
			}

			core::Path<char> filePath;
			AssetPathForRawFile(rawfileInfo, filePath);

			X_LOG1("AssetDB", "Updating compression header for raw_file %" PRIi32 " path \"%s\"", rawFileId, filePath.c_str());

			core::XFileScoped file;
			if (!file.openFile(filePath.c_str(), 
				core::fileMode::READ |
				core::fileMode::WRITE |
				core::fileMode::RANDOM_ACCESS))
			{
				X_ERROR("AssetDB", "Failed to open rawfile");
				return false;
			}

			core::Compression::BufferHdr hdr;

			if (file.readObj(hdr) != sizeof(hdr))
			{
				X_ERROR("AssetDB", "Failed to rawfile read header");
				return false;
			}

			if (!hdr.IsMagicValid())
			{
				X_ERROR("AssetDB", "Rawfile has a invalid header");
				return false;
			}


			auto it = std::find_if(algoMap.begin(), algoMap.end(), [&hdr](const AlgoEnumPair& ap) -> bool { 
				return hdr.algo == ap.first;
			});
			
			if (it == algoMap.end())
			{
				// failed to map algo
				X_ERROR("AssetDB", "Failed to map algo: \"%s\"", Algo::ToString(hdr.algo));
				return false;
			}

			// update algo.
			hdr.algo = it->second;

			file.seek(0, core::SeekMode::SET);
			if (file.writeObj(hdr) != sizeof(hdr))
			{
				X_ERROR("AssetDB", "Failed to write updated rawFile header");
				return false;
			}
		}
	}

	if (dbVersion_ < 3)
	{
		X_WARNING("AssetDB", "Performing migrations from db version %" PRIi32 " to verison 3", dbVersion_);

		sql::SqlLiteQuery qry(db_, "SELECT file_id FROM raw_files");

		DataArr data(g_AssetDBArena);

		// transaction for the update.
		sql::SqlLiteTransaction trans(db_);

		auto it = qry.begin();
		for (; it != qry.end(); ++it)
		{
			auto row = *it;

			int32_t rawFileId = row.get<int32_t>(0);
			RawFile rawfileInfo;

			if (!GetRawfileForRawId(rawFileId, rawfileInfo))
			{
				X_ERROR("AssetDB", "Failed to get rawfile path");
				return false;
			}

			core::Path<char> filePath;
			AssetPathForRawFile(rawfileInfo, filePath);

			X_LOG1("AssetDB", "Updating compression header for raw_file %" PRIi32 " path \"%s\"", rawFileId, filePath.c_str());

			core::XFileScoped file;
			if (!file.openFile(filePath.c_str(),
				core::fileMode::READ |
				core::fileMode::WRITE |
				core::fileMode::RANDOM_ACCESS))
			{
				X_ERROR("AssetDB", "Failed to open rawfile");
				return false;
			}

			struct BufferHdrOld
			{
				X_INLINE bool IsMagicValid(void) const {
					uint32_t val = (magic[0] << 16 | magic[1] << 8 | magic[2]);
					return val == core::Compression::BufferHdr::MAGIC;
				}

				core::Compression::Algo::Enum algo;
				uint8_t magic[3];

				uint32_t deflatedSize;
				uint32_t inflatedSize;
			};

			BufferHdrOld oldHdr;
			if (file.readObj(oldHdr) != sizeof(oldHdr))
			{
				X_ERROR("AssetDB", "Failed to rawfile read header");
				return false;
			}

			if (!oldHdr.IsMagicValid())
			{
				X_ERROR("AssetDB", "Rawfile has a invalid header");
				return false;
			}

			data.resize(oldHdr.deflatedSize);

			if (file.read(data.data(), data.size()) != data.size())
			{
				X_ERROR("AssetDB", "Failed to read RawFile Data");
				return false;
			}


			// need to map old header to new.
			core::Compression::BufferHdr hdr;
			hdr.algo = oldHdr.algo;
			std::memcpy(&hdr.magic, &oldHdr.magic, sizeof(oldHdr.magic));
			hdr.flags.Clear();
			hdr.sharedDictId = 0;
			hdr.deflatedSize = oldHdr.deflatedSize;
			hdr.inflatedSize = oldHdr.inflatedSize;
			

			file.seek(0, core::SeekMode::SET);
			if (file.writeObj(hdr) != sizeof(hdr))
			{
				X_ERROR("AssetDB", "Failed to write updated rawFile header");
				return false;
			}

			if (file.write(data.data(), data.size()) != data.size())
			{
				X_ERROR("AssetDB", "Failed to write RawFile data");
				return false;
			}

			core::Crc32* pCrc32 = gEnv->pCore->GetCrc32();
			uint32_t dataCrc = pCrc32->Begin();
			pCrc32->Update(&hdr, sizeof(hdr), dataCrc);
			pCrc32->Update(data.ptr(), data.size(), dataCrc);
			dataCrc = pCrc32->Finish(dataCrc);

			// need to update rawFile size colum
			sql::SqlLiteCmd cmd(db_, "UPDATE raw_files SET size = ?, hash = ?, WHERE file_id = ?");
			cmd.bind(1, safe_static_cast<int32_t>(data.size()));
			cmd.bind(2, static_cast<int32_t>(dataCrc));
			cmd.bind(3, rawFileId);

			sql::Result::Enum res = cmd.execute();
			if (res != sql::Result::OK) {
				X_ERROR("AssetDB", "Failed to update RawFileData");
				return false;
			}

		}

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

// -----------------------------------
AssetDB::Result::Enum AssetDB::AddProfile(const core::string& name)
{
	return AddProfile(name, core::string("{}"));
}

AssetDB::Result::Enum AssetDB::AddProfile(const core::string& name, const core::string& data)
{
	if (name.isEmpty()) {
		X_ERROR("AssetDB", "Profile with empty name not allowed");
		return Result::ERROR;
	}
	if (!ValidName(name)) {
		X_ERROR("AssetDB", "Profile name \"%s\" has invalid characters", name.c_str());
		return Result::ERROR;
	}

	if (ProfileExsists(name)) {
		X_ERROR("AssetDB", "Profile with name \"%s\" already exists", name.c_str());
		return Result::NAME_TAKEN;
	}

	sql::SqlLiteTransaction trans(db_);
	sql::SqlLiteCmd cmd(db_, "INSERT INTO conversion_profiles (name, data) VALUES(?,?)");
	cmd.bind(1, name.c_str());
	cmd.bind(2, data.c_str());

	int32_t res = cmd.execute();
	if (res != 0) {
		return Result::ERROR;
	}

	trans.commit();
	return Result::OK;
}

bool AssetDB::ProfileExsists(const core::string& name, ProfileId* pProfileId)
{
	sql::SqlLiteQuery qry(db_, "SELECT profile_id FROM conversion_profiles WHERE name = ?");
	qry.bind(1, name.c_str());

	const auto it = qry.begin();

	if (it == qry.end()) {
		return false;
	}
	if (pProfileId) {
		*pProfileId = (*it).get<int32_t>(0);
	}

	return true;
}

bool AssetDB::SetProfileData(const core::string& name, const core::string& data)
{
	if (!ProfileExsists(name)) {
		X_ERROR("AssetDB", "Failed to set profile data, profile \"%s\" don't exsist", name.c_str());
		return false;
	}

	sql::SqlLiteTransaction trans(db_);
	sql::SqlLiteCmd cmd(db_, "UPDATE conversion_profiles SET data = ? WHERE name = ?");
	cmd.bind(1, data.c_str());
	cmd.bind(2, name.c_str());

	sql::Result::Enum res = cmd.execute();
	if (res != sql::Result::OK) {
		return false;
	}

	trans.commit();
	return true;
}

bool AssetDB::GetProfileData(const core::string& name, core::string& dataOut)
{
	sql::SqlLiteQuery qry(db_, "SELECT data, name FROM conversion_profiles WHERE name = ?");
	qry.bind(1, name.c_str());

	sql::SqlLiteQuery::iterator it = qry.begin();

	if (it != qry.end())
	{
		auto row = *it;

		dataOut = row.get<const char*>(0);
		return true;
	}

	X_ERROR("AssetDB", "Failed to get profile data, profile \"%s\" don't exsist", name.c_str());
	return false;
}

// --------------------------------------------------------

AssetDB::Result::Enum AssetDB::AddMod(const core::string& name, const core::Path<char>& outDir)
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
	sql::SqlLiteQuery qry(db_, "SELECT mod_id FROM mods WHERE name = ?");
	qry.bind(1, name.c_str());

	sql::SqlLiteQuery::iterator it = qry.begin();

	if (it != qry.end())
	{
		modId_ = (*it).get<int32_t>(0);

		X_LOG0("AssetDB", "Mod set: \"%s\" id: %" PRIi32, name.c_str(), modId_);
		return true;
	}

	X_ERROR("AssetDB", "Failed to set mod no mod called \"%s\" exsists", name.c_str());
	return false;
}

bool AssetDB::SetMod(ModId id)
{
	sql::SqlLiteQuery qry(db_, "SELECT name FROM mods WHERE mod_id = ?");
	qry.bind(1, id);

	sql::SqlLiteQuery::iterator it = qry.begin();

	if (it != qry.end())
	{
		const char* pName = (*it).get<const char*>(0);

		modId_ = id;
		X_LOG0("AssetDB", "Mod set: \"%s\" id: %" PRIi32, pName, modId_);
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
		X_ERROR("AssetDB", "Failed to set mod id, mod \"%s\" don't exsist", name.c_str());
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

	sql::SqlLiteQuery qry(db_, "SELECT COUNT(*) FROM file_ids WHERE mod_id = ? AND type = ?");
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

	X_ERROR("AssetDB", "Failed to get asset count for mod: %" PRIi32 " Asset type: %s", modId, AssetType::ToString(type));
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

bool AssetDB::GetAssetList(AssetType::Enum type, AssetInfoArr& assetsOut)
{
	int32_t count;
	if (!GetNumAssets(type, count)) {
		return false;
	}

	// how confident are we count always be correct..
	// could resize if so.
	assetsOut.reserve(count);

	sql::SqlLiteQuery qry(db_, "SELECT file_id, parent_id, type, name FROM file_ids WHERE type = ?");
	qry.bind(1, type);

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


		func.Invoke(static_cast<ModId>(modId), core::string(pName), core::Path<char>(pOutdir));
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

		func.Invoke(static_cast<AssetType::Enum>(type), core::string(pName));
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

		func.Invoke(static_cast<AssetType::Enum>(type), core::string(pName));
	}

	return true;
}

bool AssetDB::IterateAssets(ModId modId, AssetType::Enum type, AssetDelegate func)
{
	if (modId == INVALID_MOD_ID) {
		return false;
	}

	sql::SqlLiteQuery qry(db_, "SELECT name, type, lastUpdateTime FROM file_ids WHERE type = ? AND mod_id = ?");
	qry.bind(1, type);
	qry.bind(2, modId);

	auto it = qry.begin();
	for (; it != qry.end(); ++it)
	{
		auto row = *it;

		const char* pName = row.get<const char*>(0);
		const int32_t type = row.get<int32_t>(1);

		func.Invoke(static_cast<AssetType::Enum>(type), core::string(pName));
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

		func.Invoke(type, core::string(pName));
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


bool AssetDB::GetNumAssets(int32_t& numOut)
{
	sql::SqlLiteQuery qry(db_, "SELECT COUNT(*) FROM file_ids");

	auto it = qry.begin();
	if (it != qry.end())
	{
		auto row = *it;

		const int32_t count = row.get<int32_t>(0);

		numOut = count;
		return true;
	}

	return false;
}

bool AssetDB::GetNumAssets(AssetType::Enum type, int32_t& numOut)
{
	sql::SqlLiteQuery qry(db_, "SELECT COUNT(*) FROM file_ids WHERE type = ?");
	qry.bind(1, type);

	auto it = qry.begin();
	if (it != qry.end())
	{
		auto row = *it;

		const int32_t count = row.get<int32_t>(0);

		numOut = count;
		return true;
	}

	return false;
}

bool AssetDB::GetNumAssets(ModId modId, int32_t& numOut)
{
	AssetTypeCountsArr counts;

	if (!GetAssetTypeCounts(modId, counts)) {
		return false;
	}

	numOut = 0;

	// could std::accumulate this, but aint no body got time for dat.
	for (const auto& c : counts) {
		numOut += c;
	}
	return true;
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
	if (!AssetExsists(type, name, &assetId)) {
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

	// do we ref anything?
	// if so we need to remove them.
	RefsArr refs(g_AssetDBArena);
	if (!GetAssetRefsFrom(assetId, refs)) {
		X_ERROR("AssetDB", "Failed to delete asset: %s:%s error getting ref count", AssetType::ToString(type), name.c_str());
		return Result::ERROR;
	}

	// If we are a child, it don't matter currently
	// as the child stores the link, not the parent.

	sql::SqlLiteTransaction trans(db_);

	if (refs.isNotEmpty())
	{
		core::StackString<2048, char> stm("DELETE FROM refs WHERE ref_id IN (?");
		stm.append(", ?", refs.size() - 1);
		stm.append(")");

		sql::SqlLiteCmd cmd(db_, stm.c_str());
		for (size_t i = 0; i < refs.size(); i++)
		{
			cmd.bind(static_cast<int32_t>(i + 1), refs[i].id);
		}

		sql::Result::Enum res = cmd.execute();
		if (res != sql::Result::OK) {
			X_ERROR("AssetDB", "Error deleting refs when deleting asset");
			return Result::ERROR;
		}

		// check we delete correct amount, otherwise we have some sort of logic error.
		if (db_.numChangesFromLastStmt() != safe_static_cast<int32_t>(refs.size())) {
			X_ASSERT_UNREACHABLE();
			X_ERROR("AssetDB", "Error deleting all assets refs before removing asset");
			return Result::ERROR;
		}
	}

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
			core::Path<char> path;
			path = AssetTypeRawFolder(type);
			path.toLower();
			path /= newName;
			path.replaceSeprators();

			sql::SqlLiteTransaction trans(db_);
			sql::SqlLiteCmd cmd(db_, "UPDATE raw_files SET path = ? WHERE file_id = ?");
			cmd.bind(1, path.c_str());
			cmd.bind(2, rawId);

			sql::Result::Enum res = cmd.execute();
			if (res != sql::Result::OK) {
				return Result::ERROR;
			}

			// now move the file.
			core::Path<char> newFilePath, oldFilePath;
			AssetPathForName(type, newName, rawData.hash, newFilePath); // we pass current hash as the data is not changing.
			AssetPathForRawFile(rawData, oldFilePath);


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
	const DataArr& compressedData, const core::string& argsOpt)
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
	const uint32_t dataCrc = pCrc32->GetCRC32(compressedData.ptr(), compressedData.size());
	const uint32_t argsCrc = pCrc32->GetCRC32(args.c_str(), args.length());

	rawId = INVALID_RAWFILE_ID;
	if (compressedData.isNotEmpty())
	{
		if (!core::Compression::ICompressor::validBuffer(compressedData)) {
			X_ERROR("AssetDB", "Passed invalid buffer to UpdateAsset");
			return Result::ERROR;
		}

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

	if (compressedData.isNotEmpty())
	{
		auto res = UpdateAssetRawFileHelper(trans, type, name, assetId, rawId, compressedData, dataCrc);
		if (res != Result::OK) {
			return res;
		}
	}

	// now update file info.
	core::string stmt;
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


AssetDB::Result::Enum AssetDB::UpdateAssetRawFile(int32_t assetId, const DataArr& compressedData)
{
	// we make use of the asset name and type, so the main logic is in that one.
	AssetInfo info;

	if (GetAssetInfoForAsset(assetId, info)) {
		return Result::ERROR;
	}

	return UpdateAssetRawFile(info.type, info.name, compressedData);
}

AssetDB::Result::Enum AssetDB::UpdateAssetRawFile(AssetType::Enum type, const core::string& name,
	const DataArr& compressedData)
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
	const uint32_t dataCrc = pCrc32->GetCRC32(compressedData.ptr(), compressedData.size());

	int32_t rawId = INVALID_RAWFILE_ID;
	if (compressedData.isNotEmpty())
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

	auto res = UpdateAssetRawFileHelper(trans, type, name, assetId, rawId, compressedData, dataCrc);
	if (res != Result::OK) {
		return res;
	}

	trans.commit();
	return Result::OK;
}


AssetDB::Result::Enum AssetDB::UpdateAssetRawFileHelper(const sql::SqlLiteTransactionBase& trans,
	AssetType::Enum type, const core::string& name, int32_t assetId, int32_t rawId, const DataArr& compressedData)
{
	core::Crc32* pCrc32 = gEnv->pCore->GetCrc32();
	const uint32_t dataCrc = pCrc32->GetCRC32(compressedData.ptr(), compressedData.size());

	return UpdateAssetRawFileHelper(trans, type, name, assetId, rawId, compressedData, dataCrc);
}

AssetDB::Result::Enum AssetDB::UpdateAssetRawFileHelper(const sql::SqlLiteTransactionBase& trans,
	AssetType::Enum type, const core::string& name, int32_t assetId, int32_t rawId, const DataArr& compressedData, uint32_t dataCrc)
{
	X_UNUSED(trans); // not used just ensures you have taken one.
	X_ASSERT(assetId != INVALID_ASSET_ID, "Invalid asset ID")(assetId);
	X_ASSERT(name.isNotEmpty(), "Name can't be empty")(name.length());

	// lets not allow this to be called wiht no data.
	if (compressedData.isEmpty()) {
		X_ERROR("AssetDB", "Passed empty buffer to UpdateRawFile");
		return Result::ERROR;
	}

	if (!core::Compression::ICompressor::validBuffer(compressedData)) {
		X_ERROR("AssetDB", "Passed invalid buffer to UpdateRawFile");
		return Result::ERROR;
	}


	// save the file.
	core::Path<char> path;

	{
		core::XFileScoped file;
		core::fileModeFlags mode;
		core::Path<char> filePath;

		mode.Set(core::fileMode::WRITE);
		mode.Set(core::fileMode::RECREATE);

		AssetPathForName(type, name, dataCrc, filePath);

		if (!gEnv->pFileSys->createDirectoryTree(filePath.c_str())) {
			X_ERROR("AssetDB", "Failed to create dir to save raw asset");
			return Result::ERROR;
		}

		if (!file.openFile(filePath.c_str(), mode)) {
			X_ERROR("AssetDB", "Failed to write raw asset");
			return Result::ERROR;
		}

		if (file.write(compressedData.ptr(), compressedData.size()) != compressedData.size()) {
			X_ERROR("AssetDB", "Failed to write raw asset data");
			return Result::ERROR;
		}
	}

	if (rawId == INVALID_RAWFILE_ID)
	{
		sql::SqlLiteDb::RowId lastRowId;

		path = AssetTypeRawFolder(type);
		path.toLower();
		path /= name;
		path.replaceSeprators();
		// we don't include hash here.

		// insert entry
		{
			sql::SqlLiteCmd cmd(db_, "INSERT INTO raw_files (path, size, hash) VALUES(?,?,?)");
			cmd.bind(1, path.c_str());
			cmd.bind(2, safe_static_cast<int32_t>(compressedData.size()));
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
		cmd.bind(2, safe_static_cast<int32_t>(compressedData.size()));
		cmd.bind(3, static_cast<int32_t>(dataCrc));
		cmd.bind(4, rawId);

		sql::Result::Enum res = cmd.execute();
		if (res != sql::Result::OK) {
			return Result::ERROR;
		}
	}


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
 

AssetDB::Result::Enum AssetDB::UpdateAssetThumb(AssetType::Enum type, const core::string& name, Vec2i thumbDim, Vec2i srcDim,
	const DataArr& data, core::Compression::Algo::Enum algo, core::Compression::CompressLevel::Enum lvl)
{
	int32_t assetId;

	if (!AssetExsists(type, name, &assetId)) {
		return Result::NOT_FOUND;
	}

	return UpdateAssetThumb(assetId, thumbDim, srcDim, data, algo, lvl);
}


AssetDB::Result::Enum AssetDB::UpdateAssetThumb(int32_t assetId, Vec2i thumbDim, Vec2i srcDim, const DataArr& data,
	core::Compression::Algo::Enum algo, core::Compression::CompressLevel::Enum lvl)
{
	DataArr compressed(data.getArena());

	core::StopWatch timer;

	if (!DeflateBuffer(g_AssetDBArena, data, compressed, algo, lvl)) {
		X_ERROR("AssetDB", "Failed to defalte thumb data");
		return Result::ERROR;
	}

	const auto elapsed = timer.GetMilliSeconds();
	const float percentageSize = (static_cast<float>(compressed.size()) / static_cast<float>(data.size())) * 100;

	core::HumanSize::Str sizeStr, sizeStr2;
	X_LOG2("AssetDB", "Defalated thumb %s -> %s(%.2g%%) %gms",
		core::HumanSize::toString(sizeStr, data.size()),
		core::HumanSize::toString(sizeStr2, compressed.size()),
		percentageSize,
		elapsed
	);

	return UpdateAssetThumb(assetId, thumbDim, srcDim, compressed);
}

AssetDB::Result::Enum AssetDB::UpdateAssetThumb(AssetType::Enum type, const core::string& name, Vec2i thumbDim, Vec2i srcDim, 
	const DataArr& compressedData)
{
	int32_t assetId;

	if (!AssetExsists(type, name, &assetId)) {
		return Result::NOT_FOUND;
	}

	return UpdateAssetThumb(assetId, thumbDim, srcDim, compressedData);
}

AssetDB::Result::Enum AssetDB::UpdateAssetThumb(int32_t assetId, Vec2i thumbDim, Vec2i srcDim, const DataArr& compressedData)
{
	// so my little floating goat, we gonna store the thumbs with hash names.
	// that way i don't need to rename the fuckers if i rename the asset.
	// might do same for raw assets at somepoint...

	if (!core::Compression::ICompressor::validBuffer(compressedData)) {
		X_ERROR("AssetDB", "Passed invalid buffer to UpdateAssetThumb");
		return Result::ERROR;
	}

	core::Hash::MD5 hasher;
	hasher.update(compressedData.data(), compressedData.size());
	const auto hash = hasher.finalize();

	int32_t thumbId = INVALID_THUMB_ID;
	if (compressedData.isNotEmpty())
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

	sql::SqlLiteTransaction trans(db_);

	// write new data.
	{
		core::Path<char> filePath;
		core::Hash::MD5Digest::String strBuf;

		filePath = ASSET_DB_FOLDER;
		filePath /= THUMBS_FOLDER;
		filePath.toLower();
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

			core::XFileScoped file;

			if (!file.openFile(filePath.c_str(), mode)) {
				X_ERROR("AssetDB", "Failed to write thumb");
				return Result::ERROR;
			}

			if (file.write(compressedData.ptr(), compressedData.size()) != compressedData.size()) {
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
			sql::SqlLiteCmd cmd(db_, "INSERT INTO thumbs (width, height, srcWidth, srcHeight, size, hash) VALUES(?,?,?,?,?,?)");
			cmd.bind(1, thumbDim.x);
			cmd.bind(2, thumbDim.y);
			cmd.bind(3, srcDim.x);
			cmd.bind(4, srcDim.y);
			cmd.bind(5, safe_static_cast<int32_t>(compressedData.size()));
			cmd.bind(6, &hash, sizeof(hash));

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
		sql::SqlLiteCmd cmd(db_, "UPDATE thumbs SET width = ?, height = ?, srcWidth = ?, srcHeight = ?, size = ?, hash = ?, "
			"lastUpdateTime = DateTime('now') WHERE thumb_id = ?");
		cmd.bind(1, thumbDim.x);
		cmd.bind(2, thumbDim.y);
		cmd.bind(3, srcDim.x);
		cmd.bind(4, srcDim.y);
		cmd.bind(5, safe_static_cast<int32_t>(compressedData.size()));
		cmd.bind(6, &hash, sizeof(hash));
		cmd.bind(7, thumbId);

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
	RawFile raw;

	if (!GetRawfileForId(assetId, raw)) {
		X_ERROR("AssetDB", "Failed to get rawfile info for data retrieval");
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

	const size_t size = static_cast<size_t>(raw.size);

	DataArr compressedData(g_AssetDBArena);
	compressedData.resize(size);

	if (file.read(compressedData.ptr(), size) != size) {
		X_ERROR("AssetDB", "Failed to read rawfile contents");
		return false;
	}

#if X_ENABLE_ASSERTIONS
	const auto bytesLeft = file.remainingBytes();
	X_ASSERT(bytesLeft == 0, "Failed to read whole rawAsset")(bytesLeft);
#endif // X_ENABLE_ASSERTIONS

	// decompress it.
	if (!InflateBuffer(g_AssetDBArena, compressedData, dataOut)) {
		X_ERROR("AssetDB", "Failed to read rawfile, error inflating");
		return false;
	}

	return true;
}

bool AssetDB::AssetHasRawFile(int32_t assetId, int32_t* pRawFileId)
{
	sql::SqlLiteQuery qry(db_, "SELECT raw_file FROM file_ids WHERE file_id = ?");
	qry.bind(1, assetId);

	const auto it = qry.begin();

	if (it == qry.end()) {
		return false;
	}

	auto row = *it;

	// is it set?
	if (row.columnType(0) == sql::ColumType::SNULL) {
		return false;
	}

	if (pRawFileId) {
		*pRawFileId = (*it).get<int32_t>(0);
	}

	return true;
}

bool AssetDB::AssetHasThumb(int32_t assetId)
{
	ThumbInfo info;

	if (!GetThumbInfoForId(assetId, info)) {
		return false;
	}

	return info.fileSize > 0;
}

bool AssetDB::GetThumbForAsset(int32_t assetId, ThumbInfo& info, DataArr& thumbDataOut)
{
	if (!GetThumbInfoForId(assetId, info)) {
	// just treat it as no thumb.
	//	X_ERROR("AssetDB", "Failed to get thumb info for data retrieval");
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
	if (!InflateBuffer(g_AssetDBArena, compressedData, thumbDataOut)) {
		X_ERROR("AssetDB", "Failed to read thumb, error inflating");
		return false;
	}

	return true;
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
	sql::SqlLiteQuery qry(db_, "SELECT name, file_id, type, parent_id FROM file_ids WHERE file_ids.file_id = ?");
	qry.bind(1, assetId);

	const auto it = qry.begin();

	if (it == qry.end()) {
		infoOut = AssetInfo();
		return false;
	}

	auto row = *it;

	infoOut.name = row.get<const char*>(0);
	infoOut.id = row.get<int32_t>(1);
	infoOut.type = static_cast<AssetType::Enum>(row.get<int32_t>(2));

	if (row.columnType(3) != sql::ColumType::SNULL) {
		infoOut.parentId = row.get<int32_t>(3);
	}
	else {
		infoOut.parentId = INVALID_ASSET_ID;
	}

	return true;
}

bool AssetDB::MarkAssetsStale(int32_t modId)
{
	// basically set NULL for all compiledHashes on file_ids that match this mod.
	sql::SqlLiteTransaction trans(db_);

	sql::SqlLiteCmd cmd(db_, "UPDATE file_ids SET compiledHash = NULL, lastUpdateTime = DateTime('now') WHERE mod_id = ?");
	cmd.bind(1, modId);

	sql::Result::Enum res = cmd.execute();
	if (res != sql::Result::OK) {
		return false;
	}

	trans.commit();
	return true;
}

bool AssetDB::IsAssetStale(int32_t assetId)
{
	sql::SqlLiteQuery qry(db_, "SELECT file_ids.compiledHash, file_ids.argsHash, raw_files.hash, raw_files.size FROM file_ids "
		"INNER JOIN raw_files on raw_files.file_id = file_ids.raw_file WHERE file_ids.file_id = ?");
	qry.bind(1, assetId);

	const auto it = qry.begin();

	if (it == qry.end()) {
		return false;
	}

	auto row = *it;

	// ever compiled?
	if (row.columnType(0) == sql::ColumType::SNULL) {
		return false;
	}

	const int32_t compiledHash = row.get<int32_t>(0);
	const int32_t argsHash = row.get<int32_t>(1);
	const int32_t rawFileHash = row.get<int32_t>(2);
	const int32_t rawFileSize = row.get<int32_t>(3);

	// work out if somethings changed.
	const int32_t mergedHash = core::Crc32::Combine(argsHash, rawFileHash, rawFileSize);
	
	return mergedHash == compiledHash;
}

bool AssetDB::OnAssetCompiled(int32_t assetId)
{
	if (!AssetHasRawFile(assetId)) 
	{
		sql::SqlLiteTransaction trans(db_);

		// this is more simple, just copy the args hash.
		sql::SqlLiteCmd cmd(db_, "UPDATE file_ids SET compiledHash = argsHash, lastUpdateTime = DateTime('now') WHERE file_id = ?");
		cmd.bind(1, assetId);

		sql::Result::Enum res = cmd.execute();
		if (res != sql::Result::OK) {
			return false;
		}

		trans.commit();
		return true;
	}

	sql::SqlLiteQuery qry(db_, "SELECT file_ids.argsHash, raw_files.hash, raw_files.size FROM file_ids "
		"INNER JOIN raw_files on raw_files.file_id = file_ids.raw_file WHERE file_ids.file_id = ?");
	qry.bind(1, assetId);

	const auto it = qry.begin();
	if (it == qry.end()) {
		X_ERROR("AssetDB", "Failed to find asset for updating: %" PRIi32, assetId);
		return false;
	}

	auto row = *it;
	const int32_t argsHash = row.get<int32_t>(0);
	const int32_t rawFileHash = row.get<int32_t>(1);
	const int32_t rawFileSize = row.get<int32_t>(2);

	const int32_t mergedHash = core::Crc32::Combine(argsHash, rawFileHash, rawFileSize);

	sql::SqlLiteTransaction trans(db_);

	sql::SqlLiteCmd cmd(db_, "UPDATE file_ids SET compiledHash = ?, lastUpdateTime = DateTime('now') WHERE file_id = ?");
	cmd.bind(1, mergedHash);
	cmd.bind(2, assetId);

	sql::Result::Enum res = cmd.execute();
	if (res != sql::Result::OK) {
		return false;
	}

	trans.commit();
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

bool AssetDB::GetAssetRefsFrom(int32_t assetId, AssetIdArr& refsOut)
{
	refsOut.clear();

	sql::SqlLiteQuery qry(db_, "SELECT toId from refs WHERE fromId = ?");
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

bool AssetDB::GetAssetRefsFrom(int32_t assetId, RefsArr& refsOut)
{
	refsOut.clear();

	sql::SqlLiteQuery qry(db_, "SELECT ref_id, toId, fromId from refs WHERE fromId = ?");
	qry.bind(1, assetId);

	auto it = qry.begin();
	for (; it != qry.end(); ++it)
	{
		auto row = *it;

		const int32_t ref_id = row.get<int32_t>(0);
		const int32_t toId = row.get<int32_t>(1);
		const int32_t fromId = row.get<int32_t>(2);

		refsOut.emplace_back(ref_id, toId, fromId);
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
	sql::SqlLiteQuery qry(db_, "SELECT raw_files.file_id, raw_files.path, raw_files.size, raw_files.hash FROM raw_files "
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
	dataOut.size = (*it).get<int32_t>(2);
	dataOut.hash = static_cast<uint32_t>((*it).get<int32_t>(3));
	return true;
}

bool AssetDB::GetRawfileForRawId(int32_t rawFileId, RawFile& dataOut)
{
	sql::SqlLiteQuery qry(db_, "SELECT file_id, path, size, hash FROM raw_files WHERE file_id = ?");
	qry.bind(1, rawFileId);

	const auto it = qry.begin();

	if (it == qry.end()) {
		return false;
	}

	if ((*it).columnType(1) != sql::ColumType::SNULL) {
		dataOut.path = (*it).get<const char*>(1);
	}
	else {
		dataOut.path.clear();
	}

	dataOut.file_id = (*it).get<int32_t>(0);
	dataOut.size = (*it).get<int32_t>(2);
	dataOut.hash = static_cast<uint32_t>((*it).get<int32_t>(3));
	return true;
}

bool AssetDB::GetThumbInfoForId(int32_t assetId, ThumbInfo& dataOut, int32_t* pThumbId)
{
	sql::SqlLiteQuery qry(db_, "SELECT thumbs.thumb_id, thumbs.width, thumbs.height, "
	" thumbs.srcWidth, thumbs.srcHeight, thumbs.size, thumbs.hash FROM thumbs "
	"INNER JOIN file_ids on thumbs.thumb_id = file_ids.thumb_id WHERE file_ids.file_id = ?");
	qry.bind(1, assetId);

	const auto it = qry.begin();

	if (it == qry.end()) {
		return false;
	}

	const auto& row = (*it);

	dataOut.id = row.get<int32_t>(0);
	dataOut.thumbDim.x = row.get<int32_t>(1);
	dataOut.thumbDim.y = row.get<int32_t>(2);

	dataOut.srcDim = Vec2i::zero();
	if (row.columnType(3) == sql::ColumType::INTEGER) {
		dataOut.srcDim.x = row.get<int32_t>(3);
	}
	if (row.columnType(4) == sql::ColumType::INTEGER) {
		dataOut.srcDim.y = row.get<int32_t>(4);
	}

	dataOut.fileSize = row.get<int32_t>(5);

	if (pThumbId) {
		*pThumbId = dataOut.id;
	}

	const void* pHash = row.get<void const*>(6);
	const size_t hashBlobSize = row.columnBytes(6);
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

bool AssetDB::getDBVersion(int32_t& versionOut)
{
	sql::SqlLiteQuery qry(db_, "PRAGMA user_version");

	const auto it = qry.begin();

	if (it == qry.end()) {
		return false;
	}

	versionOut = (*it).get<int32_t>(0);
	return true;
}

bool AssetDB::setDBVersion(int32_t version)
{
	core::StackString512 versionStr("PRAGMA user_version = ");
	versionStr.appendFmt("%" PRIi32 ";", DB_VERSION);
	if (!db_.execute(versionStr.c_str())) {
		return false;
	}

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

void AssetDB::AssetPathForName(AssetType::Enum type, const core::string& name, uint32_t rawDataHash, core::Path<char>& pathOut)
{
	pathOut = ASSET_DB_FOLDER;
	pathOut /= RAW_FILES_FOLDER;
	pathOut /= AssetTypeRawFolder(type);
	pathOut.toLower();
	pathOut /= name;
	pathOut.replaceSeprators();
	pathOut.appendFmt(".%" PRIu32, rawDataHash);
}

void AssetDB::AssetPathForRawFile(const RawFile& raw, core::Path<char>& pathOut)
{
	pathOut = ASSET_DB_FOLDER;
	pathOut /= RAW_FILES_FOLDER;
	pathOut /= raw.path;
	pathOut.replaceSeprators();
	pathOut.appendFmt(".%" PRIu32, raw.hash);
}

void AssetDB::ThumbPathForThumb(const ThumbInfo& thumb, core::Path<char>& pathOut)
{
	core::Hash::MD5Digest::String hashStr;

	pathOut = ASSET_DB_FOLDER;
	pathOut /= THUMBS_FOLDER;
	pathOut /= thumb.hash.ToString(hashStr);
	pathOut.replaceSeprators();
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

	// support a prefix that can only appear at start of a sub name.
	core::StringTokenizer<char> tokenize(name.begin(), name.end(), ASSET_NAME_SLASH);
	core::StringRange<char> token(nullptr, nullptr);

	while (tokenize.ExtractToken(token))
	{
		// don't allow "name\\post_double" 
		if (token.GetLength() < 1) {
			return false;
		}

		const auto len = token.GetLength();
		const auto pSrc = token.GetStart();
		
		size_t i = 0;
		if (pSrc[i] == ASSET_NAME_PREFIX) {
			++i;

			// make sure no more slashes.
			if (core::strUtil::Find(&pSrc[i], ASSET_NAME_SLASH))
			{
				X_ERROR("AssetDB", "Asset name \"%s\" has slash after prefix", name.c_str());
				return false;
			}
		}

		for (; i < len; i++)
		{
			const char ch = pSrc[i];

			bool valid = core::strUtil::IsAlphaNum(ch) || core::strUtil::IsDigit(ch) || ch == '_' || ch == ASSET_NAME_SLASH;
			if (!valid) {
				const size_t idx = (&pSrc[i] - name.begin()) + 1;
				X_ERROR("AssetDB", "Asset name \"%s\" has invalid character at position %" PRIuS, name.c_str(), idx);
				return false;
			}

			// provide more info when it's case error.
			if (core::strUtil::IsAlpha(ch) && !core::strUtil::IsLower(ch)) {
				const size_t idx = (&pSrc[i] - name.begin()) + 1; // make it none 0 index based for the plebs.
				X_ERROR("AssetDB", "Asset name \"%s\" has invalid upper-case character at position %" PRIuS, name.c_str(), idx); 
				return false;
			}
		}
	}

	return true;
}

core::Compression::ICompressor* AssetDB::AllocCompressor(core::LinearAllocator* pAllocator, core::Compression::Algo::Enum algo)
{
	core::Compression::ICompressor* pCom = nullptr;

	static_assert(core::Compression::Algo::ENUM_COUNT == 7, "Added additional compression algos? this code needs updating.");

	switch (algo)
	{
	case core::Compression::Algo::LZ4:
		pCom = core::Mem::Construct<core::Compression::Compressor<core::Compression::LZ4>>(pAllocator->allocate(sizeof(core::Compression::Compressor<core::Compression::LZ4>), X_ALIGN_OF(core::Compression::Compressor<core::Compression::LZ4>), 0));
		break;
	case core::Compression::Algo::LZ4HC:
		pCom = core::Mem::Construct<core::Compression::Compressor<core::Compression::LZ4HC>>(pAllocator->allocate(sizeof(core::Compression::Compressor<core::Compression::LZ4>), X_ALIGN_OF(core::Compression::Compressor<core::Compression::LZ4HC>), 0));
		break;
	case core::Compression::Algo::LZ5:
		pCom = core::Mem::Construct<core::Compression::Compressor<core::Compression::LZ5>>(pAllocator->allocate(sizeof(core::Compression::Compressor<core::Compression::LZ5>), X_ALIGN_OF(core::Compression::Compressor<core::Compression::LZ5>), 0));
		break;
	case core::Compression::Algo::LZ5HC:
		pCom = core::Mem::Construct<core::Compression::Compressor<core::Compression::LZ5HC>>(pAllocator->allocate(sizeof(core::Compression::Compressor<core::Compression::LZ5HC>), X_ALIGN_OF(core::Compression::Compressor<core::Compression::LZ5HC>), 0));
		break;
	case core::Compression::Algo::LZMA:
		pCom = core::Mem::Construct<core::Compression::Compressor<core::Compression::LZ4>>(pAllocator->allocate(sizeof(core::Compression::Compressor<core::Compression::LZ4>), X_ALIGN_OF(core::Compression::Compressor<core::Compression::LZ4>), 0));
		break;
	case core::Compression::Algo::ZLIB:
		pCom = core::Mem::Construct<core::Compression::Compressor<core::Compression::LZ4>>(pAllocator->allocate(sizeof(core::Compression::Compressor<core::Compression::LZ4>), X_ALIGN_OF(core::Compression::Compressor<core::Compression::LZ4>), 0));
		break;
	case core::Compression::Algo::STORE:
		pCom = core::Mem::Construct<core::Compression::Compressor<core::Compression::Store>>(pAllocator->allocate(sizeof(core::Compression::Compressor<core::Compression::Store>), X_ALIGN_OF(core::Compression::Compressor<core::Compression::Store>), 0));
		break;

	default:
		X_ASSERT_UNREACHABLE();
		break;
	}

	return pCom;
}

bool AssetDB::InflateBuffer(core::MemoryArenaBase* scratchArena, const DataArr& deflated, DataArr& inflated)
{
	X_ALIGNED_SYMBOL(char buf[MAX_COMPRESSOR_SIZE], 16);
	core::LinearAllocator allocator(buf, buf + sizeof(buf));

	if (!core::Compression::ICompressor::validBuffer(deflated)) {
		X_ERROR("AssetDB", "Tried to invalid a invalid buffer");
		return false;
	}

	const core::Compression::Algo::Enum algo = core::Compression::ICompressor::getAlgo(deflated);

	auto* pCompressor = AllocCompressor(&allocator, algo);
	if (!pCompressor) {
		X_ERROR("AssetDB", "Failed to get compressor");
		return false;
	}

	bool result = pCompressor->inflate(scratchArena, deflated, inflated);

	core::Mem::Destruct(pCompressor);

	return result;
}

bool AssetDB::DeflateBuffer(core::MemoryArenaBase* scratchArena, const DataArr& data, DataArr& deflated,
	core::Compression::Algo::Enum algo, core::Compression::CompressLevel::Enum lvl)
{
	X_ALIGNED_SYMBOL(char buf[MAX_COMPRESSOR_SIZE], 16);
	core::LinearAllocator allocator(buf, buf + sizeof(buf));

	auto* pCompressor = AllocCompressor(&allocator, algo);
	if (!pCompressor) {
		X_ERROR("AssetDB", "Failed to get compressor");
		return false;
	}

	bool result = pCompressor->deflate(scratchArena, data, deflated, lvl);

	core::Mem::Destruct(pCompressor);

	return result;
}


X_NAMESPACE_END