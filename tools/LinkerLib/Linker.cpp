#include "stdafx.h"
#include "LinkerLib.h"

#include <String\HumanDuration.h>
#include <Time\StopWatch.h>

#include <IFileSys.h>

X_LINK_LIB("engine_AssetDb")

X_NAMESPACE_BEGIN(linker)

Linker::Linker(assetDb::AssetDB& db, core::MemoryArenaBase* scratchArea) :
	scratchArea_(scratchArea),
	db_(db),
	builder_(scratchArea)
{

}

Linker::~Linker()
{

}

void Linker::PrintBanner(void)
{
	X_LOG0("Linker", "=================== V0.1 ===================");

}

bool Linker::Init(void)
{
	if (!db_.OpenDB()) {
		X_ERROR("Linker", "Failed to open AssetDb");
		return false;
	}

	return true;
}

bool Linker::dumpMeta(core::Path<char>& inputFile)
{
	return builder_.dumpMeta(inputFile);
}


bool Linker::Build(void)
{
	core::Path<char> outPath;
	outPath = "asset_pack_01";


	int32_t numAssets = 0;
	if (!db_.GetNumAssets(numAssets)) {
		X_ERROR("Converter", "Failed to get asset count");
		return false;
	}

	X_LOG0("Linker", "%" PRIi32 " asset(s)", numAssets);

	assetDb::AssetDB::AssetDelegate func;
	func.Bind<Linker, &Linker::AddAsset>(this);

	core::StopWatch timer;

	if (!db_.IterateAssets(func)) {
		X_ERROR("Linker", "Failed to convert all assets");
		return false;
	}

	core::HumanDuration::Str timeStr;
	X_LOG0("Linker", "Added %" PRIi32 " asset(s) in ^6%s", numAssets,
		core::HumanDuration::toString(timeStr, timer.GetMilliSeconds()));

	builder_.bake();
	builder_.save(outPath);

	return true;
}


bool Linker::AddAsset(assetDb::AssetType::Enum assType, const core::string& name)
{
	if (assType != assetDb::AssetType::IMG) {
		return true;
	}

	assetDb::AssetId assetId = assetDb::INVALID_ASSET_ID;
	assetDb::AssetDB::ModId modId;
	if (!db_.AssetExsists(assType, name, &assetId, &modId)) {
		X_ERROR("Linker", "Asset does not exists");
		return false;
	}

	assetDb::AssetDB::Mod modInfo;
	if (!db_.GetModInfo(modId, modInfo)) {
		X_ERROR("Linker", "Failed to get mod info");
		return false;
	}

	core::Path<char> assetPath;
	GetOutputPathForAsset(assType, name, modInfo.outDir, assetPath);

	assetPath.setExtension("ci");

	// load it.
	core::XFileScoped file;
	core::fileModeFlags mode;
	mode.Set(core::fileMode::READ);
	mode.Set(core::fileMode::SHARE);

	if (!file.openFile(assetPath.c_str(), mode)) {
		return false;
	}

	const auto fileSize = file.remainingBytes();

	core::Array<uint8_t> data(scratchArea_);
	data.resize(fileSize);

	if (file.read(data.data(), data.size()) != fileSize) {
		return false;
	}

	builder_.addAsset(assetId, name, assType, std::move(data));
	return true;
}

void GetOutputPathForAssetType(assetDb::AssetType::Enum assType,
	const core::Path<char>& modPath, core::Path<char>& pathOut)
{
	pathOut.clear();
	pathOut /= modPath;
	pathOut /= assetDb::AssetType::ToString(assType);
	pathOut += "s";
	pathOut.toLower();
	pathOut.replaceSeprators();
}

void Linker::GetOutputPathForAsset(assetDb::AssetType::Enum assType, const core::string& name,
	const core::Path<char>& modPath, core::Path<char>& pathOut)
{
	GetOutputPathForAssetType(assType, modPath, pathOut);

	pathOut /= name;
	pathOut.replaceSeprators();

	// get the extension that will be used.
	// so we can actually find the output file on disk if we want.
//	IConverter* pCon = GetConverter(assType);
//	if (pCon) {
//		pathOut.setExtension(pCon->getOutExtension());
//	}
}


X_NAMESPACE_END
