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


Converter::Converter()
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

bool Converter::Convert(AssetType::Enum assType, const core::string& name)
{
	X_LOG0("Converter", "Converting \"%s\" type: \"%s\"", name.c_str(), AssetType::ToString(assType));

	if (!db_.OpenDB()) {
		X_ERROR("Converter", "Failed to open AssetDb");
		return false;
	}

	int32_t assetId = -1;
	if (!db_.AssetExsists(assType, name, &assetId)) {
		X_ERROR("Converter", "Asset does not exists");
		return false;
	}

	core::string argsStr;
	if (!db_.GetArgsForAsset(assetId, argsStr)) {
		X_ERROR("Converter", "Failed to get conversion args");
		return false;
	}

	core::StopWatch timer;

	core::Array<uint8_t> data(g_arena);
	if(!db_.GetRawFileDataForAsset(assetId, data)) {
		return false;
	}

	X_LOG1("Converter", "Loaded rawfile in: ^6%gms", timer.GetMilliSeconds());

	core::Path<char> pathOut;
	GetOutputPathForAsset(assType, name, pathOut);

	timer.Start();

	bool res = Convert_int(assType, argsStr, data, pathOut);
	if (res) {
		X_LOG1("Converter", "processing took: ^6%gms", timer.GetMilliSeconds());
	}
	return res;
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

bool Converter::ConvertAll(AssetType::Enum assType)
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

bool Converter::CleanAll(const char* pMod)
{
	// this is for cleaning all asset that don't have a entry.
	// optionally limit it to a mod.
	assetDb::AssetDB::ModId modId;
	if (pMod) {
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


bool Converter::CleanMod(assetDb::AssetDB::ModId modId, const core::string& name, core::Path<char>& outDir)
{
	// should we just get all the assets build a hash and then check all file names
	// have a valid entry.
	// OR we can iterate the files on disk and check the DB.

	X_ASSERT_NOT_IMPLEMENTED();
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
	core::Path<char>& pathOut)
{
	pathOut.clear();
	pathOut /= AssetType::ToString(assType);
	pathOut /= "s";
	pathOut.toLower();
	pathOut.ensureSlash();

	// make sure output folder is valid.
	gEnv->pFileSys->createDirectoryTree(pathOut.c_str());

	pathOut /= name;
}

X_NAMESPACE_END