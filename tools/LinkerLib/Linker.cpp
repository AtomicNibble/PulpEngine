#include "stdafx.h"
#include "LinkerLib.h"
#include "AssetList.h"

#include <String\HumanDuration.h>
#include <Time\StopWatch.h>

#include <IFileSys.h>

X_LINK_ENGINE_LIB("AssetDb")
X_LINK_ENGINE_LIB("ConverterLib")

X_NAMESPACE_BEGIN(linker)

Linker::Linker(assetDb::AssetDB& db, core::MemoryArenaBase* scratchArea) :
    scratchArea_(scratchArea),
    db_(db),
    converter_(db, scratchArea),
    builder_(scratchArea)
{
    X_ASSERT(scratchArea->isThreadSafe(), "Scratch arena must be thread safe")(); 
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
    if (!converter_.Init()) {
        X_ERROR("Linker", "Failed to init converter");
        return false;
    }

    return true;
}

bool Linker::dumpMeta(core::Path<char>& inputFile)
{
    return builder_.dumpMeta(inputFile);
}

bool Linker::Build(BuildOptions& options)
{
    core::StopWatch timer;
    int32_t numAssets = 0;

    if (options.mod.isNotEmpty()) {
        if (!db_.SetMod(options.mod)) {
            X_LOG0("Linker", "Failed to set active mod");
            return false;
        }
    }

    if (options.assetList.isNotEmpty())
    {
        AssetList assetList(scratchArea_);

        if (!assetList.loadFromFile(options.assetList)) {
            return false;
        }

        auto& assets = assetList.getAssetList();

        numAssets = core::accumulate(assets.begin(), assets.end(), 0_i32, [](const AssetList::StringArr& list) {
            return safe_static_cast<int32_t>(list.size());
        });

        X_LOG0("Linker", "Adding %" PRIi32 " asset(s) ...", numAssets);

        for (uint32_t i = 0; i < assetDb::AssetType::ENUM_COUNT; i++)
        {
            auto assetType = static_cast<assetDb::AssetType::Enum>(i);
            auto& namesArr = assets[assetType];
            
            if (namesArr.isEmpty()) {
                continue;
            }

            for (auto& name : namesArr) {
                if (!AddAssetAndDepenency(assetType, name)) {
                    X_ERROR("Linker", "Failed to add asset");
                    return false;
                }
            }
        }
    }
    else
    {
        if (!db_.GetNumAssets(numAssets)) {
            X_ERROR("Linker", "Failed to get asset count");
            return false;
        }

        X_LOG0("Linker", "Adding %" PRIi32 " asset(s) ...", numAssets);

        assetDb::AssetDB::AssetDelegate func;
        func.Bind<Linker, &Linker::AddAsset>(this);

        if (!db_.IterateAssets(func)) {
            X_ERROR("Linker", "Failed to convert all assets");
            return false;
        }
    }

    core::HumanDuration::Str timeStr;
    X_LOG0("Linker", "Added %" PRIi32 " asset(s) in ^6%s", numAssets,
        core::HumanDuration::toString(timeStr, timer.GetMilliSeconds()));

    builder_.setFlags(options.flags);

    if (!builder_.process()) {
        X_ERROR("Linker", "Failed to bake");
        return false;
    }

    if (!builder_.save(options.outFile)) {
        X_ERROR("Linker", "Failed to save: \"%s\"", options.outFile.c_str());
        return false;
    }

    return true;
}

bool Linker::AddAssetAndDepenency(assetDb::AssetType::Enum assType, const core::string& name)
{
    assetDb::AssetId assetId = assetDb::INVALID_ASSET_ID;
    if (!db_.AssetExsists(assType, name, &assetId)) {
        X_ERROR("Linker", "Failed to get Asset id: %s \"%s\"", assetDb::AssetType::ToString(assType), name.c_str());
        return false;
    }

    core::Array<AssetDep> dependencies(scratchArea_);
    if (!converter_.GetDependencies(assetId, dependencies)) {
        X_ERROR("Linker", "Failed to get dependencies for: %s \"%s\"", assetDb::AssetType::ToString(assType), name.c_str());
        return false;
    }

    if (!AddAsset(assType, name)) {
        X_ERROR("Linker", "Failed to add asset");
        return false;
    }

    if (dependencies.isEmpty()) {
        return true;
    }

    X_LOG_BULLET;
    for (auto& dep : dependencies) {
        if (!AddAssetAndDepenency(dep.type, dep.name)) {
            X_ERROR("Linker", "Failed to add dependency for Asset: %s \"%s\"", assetDb::AssetType::ToString(assType), name.c_str());
            return false;
        }
    }

    return true;
}

bool Linker::AddAsset(assetDb::AssetType::Enum assType, const core::string& name)
{
    assetDb::AssetId assetId = assetDb::INVALID_ASSET_ID;
    assetDb::AssetDB::ModId modId;
    if (!db_.AssetExsists(assType, name, &assetId, &modId)) {
        X_ERROR("Linker", "Asset does not exists: %s \"%s\"", assetDb::AssetType::ToString(assType), name.c_str());
        return false;
    }

    if (builder_.hasAsset(assetId)) {
        X_LOG0("Linker", "skipping duplicate asset %s \"%s\"", assetDb::AssetType::ToString(assType), name.c_str());
        return true;
    }

    core::Path<char> assetPath;
    if (!db_.GetOutputPathForAsset(modId, assType, name, assetPath)) {
        X_ERROR("Linker", "Failed to get asset path");
        return false;
    }

    // load it.
    core::XFileScoped file;
    core::fileModeFlags mode;
    mode.Set(core::fileMode::READ);
    mode.Set(core::fileMode::SHARE);

    if (!file.openFile(assetPath.c_str(), mode)) {
        X_ERROR("Linker", "Failed to open asset: \"%s\"", name.c_str());
        return false;
    }

    const uint64_t realfileSize = file.remainingBytes();

    if (realfileSize > AssetPak::PAK_MAX_ASSET_SIZE) {
        core::HumanSize::Str sizeStr;
        X_ERROR("Linker", "Can't add %s \"%s\" the asset is too big: %s",
            assetDb::AssetType::ToString(assType), name.c_str(), core::HumanSize::toString(sizeStr, realfileSize));
        return false;
    }

    const auto fileSize = safe_static_cast<size_t>(realfileSize);

    core::Array<uint8_t> data(scratchArea_);
    data.resize(fileSize);

    if (file.read(data.data(), data.size()) != fileSize) {
        X_ERROR("Linker", "Failed to read data for %s \"%s\"", assetDb::AssetType::ToString(assType), name.c_str());
        return false;
    }

    core::Path<char> relAssPath;
    assetDb::AssetDB::GetRelativeOutputPathForAsset(assType, name, relAssPath);

    builder_.addAsset(assetId, name, core::string(relAssPath.begin(), relAssPath.end()), assType, std::move(data));
    return true;
}

X_NAMESPACE_END
