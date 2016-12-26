#include "stdafx.h"
#include "Converter.h"

#include <IModel.h>
#include <IAnimation.h>
#include <IConverterModule.h>
#include <IFileSys.h>

#include <Extension\IPotatoFactory.h>
#include <Extension\PotatoCreateClass.h>

#include <Containers\Array.h>

#include <String\HumanDuration.h>
#include <String\Json.h>
#include <Time\StopWatch.h>

// need assetDB.
X_LINK_LIB("engine_AssetDb")

X_NAMESPACE_BEGIN(converter)


Converter::Converter(assetDb::AssetDB& db, core::MemoryArenaBase* scratchArea) :
	scratchArea_(scratchArea),
	db_(db),
	forceConvert_(false)
{
	core::zero_object(converters_);
}

Converter::~Converter()
{
	UnloadConverters();
}

void Converter::PrintBanner(void)
{
	X_LOG0("Converter", "=================== V0.1 ===================");

}

bool Converter::Init(void)
{
	if (!db_.OpenDB()) {
		X_ERROR("Converter", "Failed to open AssetDb");
		return false;
	}

	return true;
}


void Converter::forceConvert(bool force)
{
	forceConvert_ = force;
}

bool Converter::setConversionProfiles(const core::string& profileName)
{
	X_LOG0("Converter", "Applying conversion profile: \"%s\"", profileName.c_str());

	if (!loadConversionProfiles(profileName)) {
		X_ERROR("Converter", "Failed to apply conversion profile");
		return false;
	}

	return true;
}

bool Converter::Convert(AssetType::Enum assType, const core::string& name)
{
	X_LOG0("Converter", "Converting \"%s\" type: \"%s\"", name.c_str(), AssetType::ToString(assType));

	// early out if we dont have the con lib for this ass type.
	if (!EnsureLibLoaded(assType)) {
		X_ERROR("Converter", "Failed to convert, converter module missing for asset type.");
		return false;
	}

	int32_t assetId = -1;
	assetDb::AssetDB::ModId modId;
	if (!db_.AssetExsists(assType, name, &assetId, &modId)) {
		X_ERROR("Converter", "Asset does not exists");
		return false;
	}

	assetDb::AssetDB::Mod modInfo;
	if (!db_.GetModInfo(modId, modInfo)) {
		X_ERROR("Converter", "Failed to get mod info");
		return false;
	}

	core::Path<char> pathOut;
	GetOutputPathForAsset(assType, name, modInfo.outDir, pathOut);

	// file exist already?
	if (!forceConvert_ && gEnv->pFileSys->fileExists(pathOut.c_str()))
	{
		// se if stale.
		if (db_.IsAssetStale(assetId)) {
			X_LOG1("Converter", "Skipping conversion, asset is not stale");
			return true;
		}
	}

	core::StopWatch timer;

	core::string argsStr;
	if (!db_.GetArgsForAsset(assetId, argsStr)) {
		X_ERROR("Converter", "Failed to get conversion args");
		return false;
	}

	// make sure out dir is valid.
	{
		core::Path<char> dir(pathOut);
		dir.removeFileName();
		if (!gEnv->pFileSys->directoryExists(dir.c_str()))
		{
			if (!gEnv->pFileSys->createDirectoryTree(dir.c_str()))
			{
				X_ERROR("Converter", "Failed to create output directory for asset");
				return false;
			}
		}
	}


	timer.Start();

	bool res = Convert_int(assType, assetId, argsStr, pathOut);
	if (res) {
		X_LOG1("Converter", "processing took: ^6%g ms", timer.GetMilliSeconds());

		db_.OnAssetCompiled(assetId);
	}
	return res;
}

bool Converter::Convert(int32_t modId)
{
	X_LOG0("Converter", "Converting all assets...");

	int32_t numAssets = 0;
	if (!db_.GetNumAssets(modId, numAssets)) {
		X_ERROR("Converter", "Failed to get asset count");
		return false;
	}
	
	X_LOG0("Converter", "%" PRIi32 " asset(s)", numAssets);

	core::Delegate<bool(AssetType::Enum, const core::string& name)> func;
	func.Bind<Converter, &Converter::Convert>(this);

	core::StopWatch timer;

	if (!db_.IterateAssets(modId, func)) {
		X_ERROR("Converter", "Failed to convert all assets");
		return false;
	}

	core::HumanDuration::Str timeStr;
	X_LOG0("Converter", "Converted %" PRIi32 " asset(s) in ^6%s", numAssets,
		core::HumanDuration::toString(timeStr, timer.GetMilliSeconds()));
	return true;
}

bool Converter::Convert(int32_t modId, AssetType::Enum assType)
{
	X_LOG0("Converter", "Converting all \"%s\" assets ...", AssetType::ToString(assType));

	// early out.
	if (!EnsureLibLoaded(assType)) {
		X_ERROR("Converter", "Failed to convert, converter missing for asset type.");
		return false;
	}

	int32_t numAssets = 0;
	if (!db_.GetAssetTypeCount(modId, assType, numAssets)) {
		X_ERROR("Converter", "Failed to get asset count");
		return false;
	}
	
	X_LOG0("Converter", "%" PRIi32 " asset(s)", numAssets);


	core::Delegate<bool(AssetType::Enum, const core::string& name)> func;
	func.Bind<Converter, &Converter::Convert>(this);

	core::StopWatch timer;

	if (!db_.IterateAssets(modId, assType, func)) {
		X_ERROR("Converter", "Failed to convert \"%s\" assets", AssetType::ToString(assType));
		return false;
	}

	core::HumanDuration::Str timeStr;
	X_LOG0("Converter", "Converted %" PRIi32 " asset(s) in ^6%s", numAssets, 
		core::HumanDuration::toString(timeStr, timer.GetMilliSeconds()));
	return true;
}

bool Converter::Convert(AssetType::Enum assType)
{
	X_LOG0("Converter", "Converting all \"%s\" assets ...", AssetType::ToString(assType));

	if (!EnsureLibLoaded(assType)) {
		X_ERROR("Converter", "Failed to convert, converter missing for asset type.");
		return false;
	}

	int32_t numAssets = 0;
	if (!db_.GetNumAssets(assType, numAssets)) {
		X_ERROR("Converter", "Failed to get asset count");
		return false;
	}
	
	X_LOG0("Converter", "%" PRIi32 " asset(s)", numAssets);

	core::Delegate<bool(AssetType::Enum, const core::string& name)> func;
	func.Bind<Converter, &Converter::Convert>(this);

	core::StopWatch timer;

	if (!db_.IterateAssets(assType, func)) {
		X_ERROR("Converter", "Failed to convert \"%s\" assets", AssetType::ToString(assType));
		return false;
	}

	core::HumanDuration::Str timeStr;
	X_LOG0("Converter", "Converted %" PRIi32 " asset(s) in ^6%s", numAssets,
		core::HumanDuration::toString(timeStr, timer.GetMilliSeconds()));
	return true;
}


bool Converter::ConvertAll(void)
{
	X_LOG0("Converter", "Converting all assets...");

	int32_t numAssets = 0;
	if (db_.GetNumAssets(numAssets)) {
		X_ERROR("Converter", "Failed to get asset count");
		return false;
	}

	X_LOG0("Converter", "%" PRIi32 " asset(s)", numAssets);

	core::Delegate<bool(AssetType::Enum, const core::string& name)> func;
	func.Bind<Converter, &Converter::Convert>(this);

	core::StopWatch timer;

	if (!db_.IterateAssets(func)) {
		X_ERROR("Converter", "Failed to convert all assets");
		return false;
	}

	core::HumanDuration::Str timeStr;
	X_LOG0("Converter", "Converted %" PRIi32 " asset(s) in ^6%s", numAssets,
		core::HumanDuration::toString(timeStr, timer.GetMilliSeconds()));
	return true;
}

bool Converter::CleanAll(const char* pMod)
{
	if (pMod) {
		X_LOG0("Converter", "Cleaning all compiled assets for mod: \"%s\"", pMod);
	}
	else {
		X_LOG0("Converter", "Cleaning all compiled assets");
	}

	// this is for cleaning all asset that don't have a entry.
	// optionally limit it to a mod.
	if (pMod) {
		assetDb::AssetDB::ModId modId;
		if (!db_.ModExsists(core::string(pMod), &modId)) {
			X_ERROR("Converter", "Can't clean mod \"%s\" does not exsist", pMod);
			return false;
		}

		assetDb::AssetDB::Mod mod;

		if (!db_.GetModInfo(modId, mod)) {
			X_ERROR("Converter", "Failed to get mod info");
			return false;
		}

		return CleanMod(modId, mod.name, mod.outDir);
	}

	core::Delegate<bool(assetDb::AssetDB::ModId id, const core::string& name, core::Path<char>& outDir)> func;
	func.Bind<Converter, &Converter::CleanMod>(this);

	return db_.IterateMods(func);
}

bool Converter::CleanAll(int32_t modId)
{
	assetDb::AssetDB::Mod mod;

	if (!db_.GetModInfo(modId, mod)) {
		X_ERROR("Converter", "Failed to get mod info");
		return false;
	}

	return CleanMod(modId, mod.name, mod.outDir);
}

bool Converter::CleanMod(assetDb::AssetDB::ModId modId, const core::string& name, core::Path<char>& outDir)
{
	// mark all the assets for this mod stale.
	if (!db_.MarkAssetsStale(modId)) {
		X_ERROR("Converter", "Failed to mark mod \"%s\" assets as state", name.c_str());
		return false;
	}

	X_LOG0("Converter", "Cleaning all compiled assets for mod: \"%s\"", name.c_str());

	core::IFileSys* pFileSys = gEnv->pFileSys;

	// nuke the output directory. BOOM!
	// if they put files in here that are custom. RIP.
	if (pFileSys->directoryExists(outDir.c_str())) 
	{
		// lets be nice and only clear dir's we actually populate.
		for (int32_t i = 0; i < AssetType::ENUM_COUNT; i++)
		{
			AssetType::Enum assType = static_cast<AssetType::Enum>(i);

			core::Path<char> assetPath;
			GetOutputPathForAssetType(assType, outDir, assetPath);

			if (pFileSys->directoryExists(assetPath.c_str()))
			{
				if (!pFileSys->deleteDirectoryContents(assetPath.c_str())) {
					X_ERROR("Converter", "Failed to clear mod \"%s\" \"%s\" assets directory", assetPath.c_str(), AssetType::ToString(assType));
					return false;
				}
			}
		}

		X_LOG0("Converter", "Cleaning complete");
	}
	else
	{
		X_WARNING("Converter", "mod dir not fnd cleaning skipped: \"\"", outDir.c_str());
	}
	
	return true;
}


bool Converter::GenerateThumbs(void)
{
	X_LOG0("Converter", "Generating thumbs");

	for (int32_t i = 0; i < AssetType::ENUM_COUNT; i++)
	{
		AssetType::Enum assType = static_cast<AssetType::Enum>(i);

		if (!EnsureLibLoaded(assType)) {
			X_LOG0("Converter", "Skipping \"%s\" thumb generation, no converter module found", AssetType::ToString(assType));
			continue;
		}

		IConverter* pCon = GetConverter(assType);
		X_ASSERT_NOT_NULL(pCon);

		if (!pCon->thumbGenerationSupported()) {
			X_LOG0("Converter", "Skipping \"%s\" thumb generation, not supported", AssetType::ToString(assType));
			continue;
		}


		int32_t numAssets = 0;
		if (!db_.GetNumAssets(assType, numAssets)) {
			X_ERROR("Converter", "Failed to get asset count");
			return false;
		}

		X_LOG0("Converter", "Generating %" PRIi32 " thumb(s)", numAssets);

		core::Delegate<bool(AssetType::Enum, const core::string& name)> func;
		func.Bind<Converter, &Converter::GenerateThumb>(this);

		core::StopWatch timer;

		if (!db_.IterateAssets(assType, func)) {
			X_ERROR("Converter", "Failed to generate thumbs for assetType \"%s\"", AssetType::ToString(assType));
			return false;
		}

		core::HumanDuration::Str timeStr;
		X_LOG0("Converter", "Generated thumbs for %" PRIi32 " asset(s) in ^6%s", numAssets,
			core::HumanDuration::toString(timeStr, timer.GetMilliSeconds()));
		
	}

	return true;
}


bool Converter::GenerateThumb(AssetType::Enum assType, const core::string& name)
{
	int32_t assetId = -1;
	if (!db_.AssetExsists(assType, name, &assetId)) {
		X_ERROR("Converter", "Asset does not exists");
		return false;
	}

	if (db_.AssetHasThumb(assetId)) {
		return true;
	}

	IConverter* pCon = GetConverter(assType);
	// this is a private member which should have this stuff validated
	// before calling this.
	X_ASSERT_NOT_NULL(pCon);
	X_ASSERT(pCon->thumbGenerationSupported(), "thumb generatino not supported")();

	core::StopWatch timer;

	if (!pCon->CreateThumb(*this, assetId, Vec2i(64,64))) {
		X_ERROR("Converter", "Failed to generate thumb for \"%s\" \"%s\"", name.c_str(), AssetType::ToString(assType));
		return false;
	}

	core::HumanDuration::Str timeStr;
	X_LOG0("Converter", "generated thumb for \"%s\" in ^6%s", name.c_str(),
		core::HumanDuration::toString(timeStr, timer.GetMilliSeconds()));
	return true;
}

bool Converter::Convert_int(AssetType::Enum assType, int32_t assetId, ConvertArgs& args, const OutPath& pathOut)
{
	IConverter* pCon = GetConverter(assType);

	if (pCon) {
		return pCon->Convert(*this, assetId, args, pathOut);
	}

	return false;
}

bool Converter::GetAssetData(int32_t assetId, core::Array<uint8_t>& dataOut)
{
	if (!db_.GetRawFileDataForAsset(assetId, dataOut)) {
		X_ERROR("Converter", "Failed to get raw data for asset: %" PRIi32, assetId);
		return false;
	}

	return true;
}


bool Converter::GetAssetData(const char* pAssetName, AssetType::Enum assType, core::Array<uint8_t>& dataOut)
{
	int32_t assetId = -1;
	if (!db_.AssetExsists(assType, core::string(pAssetName), &assetId)) {
		X_ERROR("Converter", "Asset does not exists: \"%s\"", pAssetName);
		return false;
	}

	if (!db_.GetRawFileDataForAsset(assetId, dataOut)) {
		X_ERROR("Converter", "Failed to get raw data for: \"%s\"", pAssetName);
		return false;
	}

	return true;
}

bool Converter::AssetExists(const char* pAssetName, assetDb::AssetType::Enum assType)
{
	if (!db_.AssetExsists(assType, core::string(pAssetName))) {
		return false;
	}

	return true;
}

bool Converter::UpdateAssetThumb(int32_t assetId, Vec2i thumbDim, Vec2i srcDim, const DataArr& data)
{
	auto res = db_.UpdateAssetThumb(assetId, thumbDim, srcDim, data);
	if (res != assetDb::AssetDB::Result::OK) {
		return false;
	}
	
	return true;
}


bool Converter::getConversionProfileData(assetDb::AssetType::Enum type, core::string& strOut)
{
	if (conversionProfiles_[type].isEmpty()) {
		return false;
	}

	strOut = conversionProfiles_[type];
	return true;
}

core::MemoryArenaBase* Converter::getScratchArena(void)
{
	return scratchArea_;
}

bool Converter::loadConversionProfiles(const core::string& profileName)
{
	core::string profileData;

	if (!db_.GetProfileData(profileName, profileData)) {
		return false;
	}

	clearConversionProfiles();

	// we have a json doc with objects for each asset type.
	core::json::Document d;
	d.ParseInsitu(const_cast<char*>(profileData.c_str()));

	for (auto it = d.MemberBegin(); it != d.MemberEnd(); ++it)
	{
		const auto& name = it->name;
		const auto& val = it->value;

		if (val.GetType() != core::json::kObjectType) {
			X_ERROR("Converter", "Conversion profile contains invalid data, expected object got: %" PRIu32, val.GetType());
			return false;
		}

		// now try match the name to a type.
		core::StackString<64, char> nameStr(name.GetString(), name.GetString() + name.GetStringLength());
		nameStr.toUpper();

		assetDb::AssetType::Enum type;
		int32_t i;
		for (i = 0; i < assetDb::AssetType::ENUM_COUNT; i++)
		{
			const char* pTypeStr = assetDb::AssetType::ToString(i);
			if (nameStr.isEqual(pTypeStr))
			{
				type = static_cast<assetDb::AssetType::Enum>(i);
				break;
			}
		}

		if (i == assetDb::AssetType::ENUM_COUNT) {
			X_ERROR("Converter", "Conversion profile contains unkown asset type \"%s\"", nameStr.c_str());
			return false;
		}

		X_ASSERT(type >= 0 && static_cast<uint32_t>(type) < assetDb::AssetType::ENUM_COUNT, "Invalid type")(type);

		// now we want to split this into a seperate doc.
		core::json::StringBuffer s;
		core::json::Writer<core::json::StringBuffer> writer(s);

		val.Accept(writer);

		conversionProfiles_[type] = core::string(s.GetString(), s.GetSize());
	}

	return true;
}


void Converter::clearConversionProfiles(void)
{
	for (auto& p : conversionProfiles_)
	{
		p.clear();
	}
}

IConverter* Converter::GetConverter(AssetType::Enum assType)
{
	if (!EnsureLibLoaded(assType)) {
		X_ERROR("Converter", "Failed to load convert for asset type: \"%s\"", AssetType::ToString(assType));
		return false;
	}

	return converters_[assType];
}

bool Converter::EnsureLibLoaded(AssetType::Enum assType)
{
	// this needs to be more generic.
	// might move to a single interface for all converter libs.
	if (converters_[assType]) {
		return true;
	}


	return IntializeConverterModule(assType);
}


bool Converter::IntializeConverterModule(AssetType::Enum assType)
{
	core::StackString<128> dllName("Engine_");

	{
		core::StackString<64> typeName(AssetType::ToString(assType));
		typeName.toLower();
		if (typeName.isNotEmpty()) {
			typeName[0] = ::toupper(typeName[0]);
		}

		dllName.append(typeName.c_str());
	}

	dllName.append("Lib");

	core::StackString<128> className(dllName);

	return IntializeConverterModule(assType, dllName.c_str(), className.c_str());
}

bool Converter::IntializeConverterModule(AssetType::Enum assType, const char* pDllName, const char* pModuleClassName)
{
	X_ASSERT(converters_[assType] == nullptr, "converter already init")(pDllName, pModuleClassName);

	IConverterModule* pConvertModuleOut = nullptr;
	IConverter* pConverterInstance = nullptr;

	bool result = gEnv->pCore->IntializeLoadedConverterModule(pDllName, pModuleClassName, &pConvertModuleOut, &pConverterInstance);
	if (!result) {
		return false;
	}

	// save for cleanup.
	converterModules_[assType] = pConvertModuleOut;
	converters_[assType] = pConverterInstance;

	return true;
}

void Converter::UnloadConverters(void)
{
	for (uint32_t i = 0; i < assetDb::AssetType::ENUM_COUNT; i++)
	{
		if (converters_[i])
		{
			X_ASSERT(converterModules_[i] != nullptr, "Have a converter interface without a corrisponding moduleInterface")();

			// con modules are ref counted so we can't free ourself.
			gEnv->pCore->FreeConverterModule(converterModules_[i]);

			converterModules_[i] = nullptr;
			converters_[i] = nullptr;
		}
	}
}

void Converter::GetOutputPathForAssetType(AssetType::Enum assType,
	const core::Path<char>& modPath, core::Path<char>& pathOut)
{
	pathOut.clear();
	pathOut /= modPath;
	pathOut /= AssetType::ToString(assType);
	pathOut += "s";
	pathOut.toLower();
	pathOut.replaceSeprators();
}

void Converter::GetOutputPathForAsset(AssetType::Enum assType, const core::string& name,
	const core::Path<char>& modPath, core::Path<char>& pathOut)
{
	GetOutputPathForAssetType(assType, modPath, pathOut);

	// make sure output folder is valid.
	gEnv->pFileSys->createDirectoryTree(pathOut.c_str());

	pathOut /= name;
	pathOut.replaceSeprators();

	// get the extension that will be used.
	// so we can actually find the output file on disk if we want.
	IConverter* pCon = GetConverter(assType);
	if (pCon) {
		pathOut.setExtension(pCon->getOutExtension());
	}
}

X_NAMESPACE_END