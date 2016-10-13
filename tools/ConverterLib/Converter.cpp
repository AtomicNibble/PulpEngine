#include "stdafx.h"
#include "Converter.h"

#include <IModel.h>
#include <IAnimation.h>
#include <IConverterModule.h>

#include <Extension\IPotatoFactory.h>
#include <Extension\PotatoCreateClass.h>

#include <Containers\Array.h>
#include <Platform\Module.h>

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

}

void Converter::PrintBanner(void)
{
	X_LOG0("Converter", "=================== V0.1 ===================");

}

void Converter::forceConvert(bool force)
{
	forceConvert_ = force;
}

bool Converter::Convert(AssetType::Enum assType, const core::string& name)
{
	X_LOG0("Converter", "Converting \"%s\" type: \"%s\"", name.c_str(), AssetType::ToString(assType));

	if (!db_.OpenDB()) {
		X_ERROR("Converter", "Failed to open AssetDb");
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
			X_LOG1("Converter", "Skipping compiled asset is not stale");
			return true;
		}
	}

	core::StopWatch timer;

	core::string argsStr;
	if (!db_.GetArgsForAsset(assetId, argsStr)) {
		X_ERROR("Converter", "Failed to get conversion args");
		return false;
	}

	core::Array<uint8_t> data(scratchArea_);
	if (!db_.GetRawFileDataForAsset(assetId, data)) {
		return false;
	}

	X_LOG1("Converter", "Loaded rawfile in: ^6%gms", timer.GetMilliSeconds());


	timer.Start();

	bool res = Convert_int(assType, argsStr, data, pathOut);
	if (res) {
		X_LOG1("Converter", "processing took: ^6%gms", timer.GetMilliSeconds());

		db_.OnAssetCompiled(assetId);
	}
	return res;
}


bool Converter::Convert(int32_t modId)
{
	X_LOG0("Converter", "Converting all assets...");

	if (!db_.OpenDB()) {
		X_ERROR("Converter", "Failed to open AssetDb");
		return false;
	}

	int32_t numAssets = 0;
	if (db_.GetNumAssets(modId, numAssets)) {
		X_LOG0("Converter", "%" PRIi32 " asset(s)", numAssets);
	}

	core::Delegate<bool(AssetType::Enum, const core::string& name)> func;
	func.Bind<Converter, &Converter::Convert>(this);

	if (!db_.IterateAssets(modId, func)) {
		X_ERROR("Converter", "Failed to convert all assets");
		return false;
	}

	return true;
}

bool Converter::Convert(int32_t modId, AssetType::Enum assType)
{
	X_LOG0("Converter", "Converting all \"%s\" assets ...", AssetType::ToString(assType));

	if (!db_.OpenDB()) {
		X_ERROR("Converter", "Failed to open AssetDb");
		return false;
	}

	int32_t numAssets = 0;
	if (db_.GetAssetTypeCount(modId, assType, numAssets)) {
		X_LOG0("Converter", "%" PRIi32 " asset(s)", numAssets);
	}

	core::Delegate<bool(AssetType::Enum, const core::string& name)> func;
	func.Bind<Converter, &Converter::Convert>(this);

	if (!db_.IterateAssets(modId, assType, func)) {
		X_ERROR("Converter", "Failed to convert \"%s\" assets", AssetType::ToString(assType));
		return false;
	}

	return true;
}

bool Converter::Convert(AssetType::Enum assType)
{
	X_LOG0("Converter", "Converting all \"%s\" assets ...", AssetType::ToString(assType));

	if (!db_.OpenDB()) {
		X_ERROR("Converter", "Failed to open AssetDb");
		return false;
	}

	int32_t numAssets = 0;
	if (db_.GetNumAssets(assType, &numAssets)) {
		X_LOG0("Converter", "%" PRIi32 " asset(s)", numAssets);
	}

	core::Delegate<bool(AssetType::Enum, const core::string& name)> func;
	func.Bind<Converter, &Converter::Convert>(this);

	if (!db_.IterateAssets(assType, func)) {
		X_ERROR("Converter", "Failed to convert \"%s\" assets", AssetType::ToString(assType));
		return false;
	}

	return true;
}


bool Converter::ConvertAll(void)
{
	X_LOG0("Converter", "Converting all assets...");

	if (!db_.OpenDB()) {
		X_ERROR("Converter", "Failed to open AssetDb");
		return false;
	}

	int32_t numAssets = 0;
	if (db_.GetNumAssets(&numAssets)) {
		X_LOG0("Converter", "%" PRIi32 " asset(s)", numAssets);
	}

	core::Delegate<bool(AssetType::Enum, const core::string& name)> func;
	func.Bind<Converter, &Converter::Convert>(this);

	if (!db_.IterateAssets(func)) {
		X_ERROR("Converter", "Failed to convert all assets");
		return false;
	}

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

	// nuke the output directory. BOOM!
	// if they put files in here that are custom. RIP.
	if (!gEnv->pFileSys->deleteDirectory(outDir.c_str(), true)) {
		X_ERROR("Converter", "Failed to clear mod \"%s\" assets directory", name.c_str());
	}

	return true;
}

bool Converter::Convert_int(AssetType::Enum assType, ConvertArgs& args, const core::Array<uint8_t>& fileData,
	const OutPath& pathOut)
{
	IConverter* pCon = GetConverter(assType);

	if (pCon) {
		return pCon->Convert(*this, args, fileData, pathOut);
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
	const char* pAssTypeStr = nullptr;

	if (assType == AssetType::ANIM) {
		pAssTypeStr = "Anim";
	}
	else if (assType == AssetType::MODEL) {
		pAssTypeStr = "Model";
	}
	else if (assType == AssetType::MATERIAL) {
		pAssTypeStr = "Material";
	}
	else if (assType == AssetType::IMG) {
		pAssTypeStr = "Img";
	}
	else {
		X_ASSERT_UNREACHABLE();
	}

	core::StackString<64> dllName("Engine_");
	dllName.append(pAssTypeStr);
	dllName.append("Lib");

	core::StackString<64> className(dllName);

	return IntializeConverterModule(assType, dllName.c_str(), className.c_str());
}

bool Converter::IntializeConverterModule(AssetType::Enum assType, const char* dllName, const char* moduleClassName)
{
	core::Path<char> path(dllName);

	path.setExtension(".dll");

#if !defined(X_LIB) 
	void* hModule = LoadDLL(path.c_str());
	if (!hModule) {
		if (gEnv && gEnv->pLog) {
			X_ERROR("Converter", "Failed to load converter module: %s", dllName);
		}
		return false;
	}

#endif // #if !defined(X_LIB) 


	std::shared_ptr<IConverterModule> pModule;
	if (PotatoCreateClassInstance(moduleClassName, pModule))
	{
		converters_[assType] = pModule->Initialize();
	}
	else
	{
		X_ERROR("Converter", "failed to find interface: %s -> %s", dllName, moduleClassName);
		return false;
	}

	return converters_[assType] != nullptr;
}

void* Converter::LoadDLL(const char* dllName)
{
	void* hDll = core::Module::Load(dllName);

	if (!hDll) {
		return nullptr;
	}

	ModuleLinkfunc::Pointer pfnModuleInitISystem = reinterpret_cast<ModuleLinkfunc::Pointer>(
		core::Module::GetProc(hDll, "LinkModule"));

	if (pfnModuleInitISystem)
	{
		pfnModuleInitISystem(gEnv->pCore, dllName);
	}

	return hDll;
}

void Converter::GetOutputPathForAsset(AssetType::Enum assType, const core::string& name,
	const core::Path<char>& modPath, core::Path<char>& pathOut)
{
	pathOut.clear();
	pathOut /= modPath;
	pathOut.ensureSlash();
	pathOut /= AssetType::ToString(assType);
	pathOut /= "s";
	pathOut.toLower();
	pathOut.ensureSlash();

	// make sure output folder is valid.
	gEnv->pFileSys->createDirectoryTree(pathOut.c_str());

	pathOut /= name;

	// get the extension that will be used.
	// so we can actually find the output file on disk if we want.
	IConverter* pCon = GetConverter(assType);
	if (pCon) {
		pathOut.setExtension(pCon->getOutExtension());
	}
}

X_NAMESPACE_END