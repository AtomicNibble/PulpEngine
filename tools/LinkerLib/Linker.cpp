#include "stdafx.h"
#include "LinkerLib.h"
#include "AssetList.h"

#include <String\Json.h>
#include <String\HumanDuration.h>
#include <Time\StopWatch.h>
#include <Hashing\Fnva1Hash.h>

#include <IFileSys.h>
#include <ILevel.h>

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

    return true;
}

bool Linker::dumpMetaOS(core::Path<wchar_t>& osPath)
{
    return builder_.dumpMetaOS(osPath);
}

bool Linker::Build(BuildOptions& options)
{
    core::StopWatch timer;

    if (!converter_.Init()) {
        X_ERROR("Linker", "Failed to init converter");
        return false;
    }

    if (options.mod.isNotEmpty()) {
        if (!db_.SetMod(options.mod)) {
            X_LOG0("Linker", "Failed to set active mod");
            return false;
        }
    }

    if (options.level.isNotEmpty())
    {
        // so if we have a level look for level list and manual list.
        core::Path<char> level;
        core::Path<char> levelEntDesc;
        core::Path<char> levelAssList;
        core::Path<char> levelAssListInc;

        // TODO: remove hardcoded mod.
        level.setFmt("core_assets/levels/%s.%s", options.level.c_str(), level::LVL_FILE_EXTENSION);
        levelEntDesc.setFmt("core_assets/levels/%s.%s", options.level.c_str(), level::LVL_ENT_FILE_EXTENSION);
        levelAssList.setFmt("core_assets/levels/%s.%s", options.level.c_str(), assetDb::ASSET_LIST_EXT);
        levelAssListInc.setFmt("core_assets/levels/%s_inc.%s", options.level.c_str(), assetDb::ASSET_LIST_EXT);

        // load the level.
        if (!AddAssetFromDisk(assetDb::AssetType::LEVEL, options.level + "." + level::LVL_FILE_EXTENSION, level)) {
            X_ERROR("Linker", "Failed to add level file");
            return false;
        }
        if (!AddAssetFromDisk(assetDb::AssetType::LEVEL, options.level + "." + level::LVL_ENT_FILE_EXTENSION, levelEntDesc)) {
            X_ERROR("Linker", "Failed to add level ent desc");
            return false;
        }

        if (!AddEntDesc(levelEntDesc)) {
            X_ERROR("Linker", "Failed to add level ent desc");
            return false;
        }

        if (!AddAssetList(levelAssList)) {
            X_ERROR("Linker", "Failed to add level asset list");
            return false;
        }

        // optional hardcoded list.
        if (gEnv->pFileSys->fileExists(levelAssListInc)) {
            if (!AddAssetList(levelAssListInc)) {
                X_ERROR("Linker", "Failed to add level asset inc list");
                return false;
            }
        }
    }
    else if (options.assetList.isNotEmpty())
    {
        if (!AddAssetList(options.assetList)) {
            X_ERROR("Linker", "Failed to add asset list");
            return false;
        }
    }
    else
    {
        int32_t numAssets = 0;
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
    X_LOG0("Linker", "Added asset(s) in ^6%s", core::HumanDuration::toString(timeStr, timer.GetMilliSeconds()));

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

bool Linker::AddEntDesc(core::Path<char>& inputFile)
{
    core::Array<uint8_t> data(scratchArea_);

    if (!LoadFile(inputFile, data)) {
        X_ERROR("Ents", "Failed to load ent desc");
        return false;
    }

    auto* pBegin = reinterpret_cast<const char*>(data.data());
    auto* pEnd = reinterpret_cast<const char*>(data.data() + data.size());

    // rerad me some ents!
    core::json::MemoryStream ms(pBegin, data.size());
    core::json::EncodedInputStream<core::json::UTF8<>, core::json::MemoryStream> is(ms);

    core::json::Document d;

    if (d.ParseStream<core::json::kParseCommentsFlag>(is).HasParseError()) {
        auto err = d.GetParseError();
        const char* pErrStr = core::json::GetParseError_En(err);
        size_t offset = d.GetErrorOffset();
        size_t line = core::strUtil::LineNumberForOffset(pBegin, pEnd, offset);

        X_ERROR("Linker", "Failed to parse ent desc(%" PRIi32 "): Offset: %" PRIuS " Line: %" PRIuS " Err: %s",
            err, offset, line, pErrStr);
        return false;
    }


    if (d.GetType() != core::json::Type::kObjectType) {
        X_ERROR("Ents", "Unexpected type");
        return false;
    }

    using namespace core::Hash::Literals;

    core::string assName;

    for (auto it = d.MemberBegin(); it != d.MemberEnd(); ++it)
    {
        auto& v = *it;

        if (v.name == "ents") 
        {
            if (v.value.GetType() != core::json::Type::kArrayType) {
                X_ERROR("Ents", "Ent data must be array");
                return false;
            }

            auto ents = v.value.GetArray();
            for (auto& entDesc : ents) 
            {
                if (entDesc.GetType() != core::json::Type::kObjectType) {
                    X_ERROR("Ents", "Ent description must be a object");
                    return false;
                }

                for (auto entIt = entDesc.MemberBegin(); entIt != entDesc.MemberEnd(); ++entIt) {
                    const auto& name = entIt->name;
                    const auto& value = entIt->value;

                    switch (core::Hash::Fnv1aHash(name.GetString(), name.GetStringLength())) 
                    {
                        case "Mesh"_fnv1a: {
                            if (!value.HasMember("name")) {
                                continue;
                            }

                            auto& meshName = value["name"];
                            assName.assign(meshName.GetString(), meshName.GetStringLength());

                            if (!AddAssetAndDepenency(assetDb::AssetType::MODEL, assName)) {
                                X_ERROR("Ents", "Failed to add asset from entDesc");
                               return false;
                            }

                            break;
                        }
                        case "Emitter"_fnv1a: {
                            if (!value.HasMember("effect")) {
                                continue;
                            }

                            auto& efxName = value["effect"];
                            assName.assign(efxName.GetString(), efxName.GetStringLength());
                          
                            if (!AddAssetAndDepenency(assetDb::AssetType::FX, assName)) {
                                X_ERROR("Ents", "Failed to add asset from entDesc");
                                return false;
                            }

                            break;
                        }
    
                        default:
                            break;

                    }
                }
            }
        }
        else 
        {
            X_ERROR("Linker", "Unknown ent data member: \"%s\"", v.name.GetString());
            return false;
        }
    }

    return true;
}

bool Linker::AddAssetList(core::Path<char>& inputFile)
{
    AssetList assetList(scratchArea_);

    if (!assetList.loadFromFile(inputFile)) {
        return false;
    }

    auto& assets = assetList.getAssetList();

    int32_t numAssets = core::accumulate(assets.begin(), assets.end(), 0_i32, [](const AssetList::StringArr& list) {
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

    auto& dirs = assetList.getDirList();
    for (const auto& dir : dirs)
    {
        // want to just add load of fooking files!
        assetDb::AssetDB::Mod mod;
        if (!db_.GetModInfo(db_.GetcurrentModId(), mod)) {
            X_ERROR("Linker", "Failed to get mod info");
            return false;
        }

        core::Path<> dirPath;
        assetDb::AssetDB::GetOutputPathForAssetType(dir.type, mod.outDir, dirPath);
        dirPath /= dir.path;
        dirPath.ensureSlash();

        int32_t numAdded = 0;

        if (!AddAssetDir(dir, dir.path, dirPath, numAdded)) {
            return false;
        }

        X_LOG0("Linker", "Added %" PRIi32 " asset(s) for dir ^5%s^7 : \"%s\"", 
            numAdded, assetDb::AssetType::ToString(dir.type), dir.path.c_str());
    }

    return true;
}

bool Linker::AddAssetDir(const DirEntry& dir, const core::Path<>& relPath, const core::Path<>& dirPath, int32_t& numAdded)
{
    auto* pFileSys = gEnv->pFileSys;

    core::Path<> dirSearch(dirPath);
    dirSearch.ensureSlash();
    dirSearch.append("*");

    core::IFileSys::FindData fd;
    auto findPair = pFileSys->findFirst(dirSearch, fd);

    if (findPair.handle == core::IFileSys::INVALID_FIND_HANDLE) {
        if (findPair.valid) {
            X_WARNING("Linker", "empty dir: \"%s\"", dirPath.c_str());
            return true;
        }
        
        X_ERROR("Linker", "Failed to iterate dir: \"%s\"", dirPath.c_str());
        return false;
    }

    core::Path<> basePath(dirPath);

    do
    {
        if (fd.attrib.IsSet(core::FindData::AttrFlag::DIRECTORY)) {
            core::Path<> subDir(dirPath);
            subDir /= fd.name;

            auto rel = relPath / core::Path<>(fd.name);
            rel.ensureSlash();

            if (!AddAssetDir(dir, rel, subDir, numAdded)) {
                return false;
            }
            continue;
        }

        core::string name;
        name.append(relPath.begin(), relPath.end());
        name.append(fd.name.begin(), fd.name.end());

        auto path = basePath / fd.name;

        if (!AddAssetFromDisk(dir.type, name, path)) {
            X_ERROR("Linker", "Failed to add: %s", name.c_str());
            return false;
        }

        ++numAdded;

    } while (pFileSys->findnext(findPair.handle, fd));

    pFileSys->findClose(findPair.handle);
    return true;
}

bool Linker::AddAssetAndDepenency(assetDb::AssetType::Enum assType, const core::string& name)
{
    assetDb::AssetId assetId = assetDb::INVALID_ASSET_ID;
    if (!db_.AssetExists(assType, name, &assetId)) {
        X_ERROR("Linker", "Failed to get Asset id: %s \"%s\"", assetDb::AssetType::ToString(assType), name.c_str());
        return false;
    }

    if (builder_.hasAsset(assetId)) {
        X_LOG0("Linker", "skipping duplicate asset %s \"%s\"", assetDb::AssetType::ToString(assType), name.c_str());
        return true;
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
    if (!db_.AssetExists(assType, name, &assetId, &modId)) {
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
    core::FileFlags mode;
    mode.Set(core::FileFlag::READ);
    mode.Set(core::FileFlag::SHARE);

    if (!file.openFile(assetPath, mode)) {
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

    builder_.addAsset(assetId, assType, name, std::move(data));
    return true;
}

bool Linker::AddAssetFromDisk(assetDb::AssetType::Enum assType, const core::string& name, const core::Path<char>& path)
{
    core::Array<uint8_t> data(scratchArea_);

    if (!LoadFile(path, data)) {
        X_ERROR("Linker", "Failed to read data for %s \"%s\"", assetDb::AssetType::ToString(assType), name.c_str());
        return false;
    }

    builder_.addAsset(assetDb::INVALID_ASSET_ID, assType, name, std::move(data));
    return true;
}

bool Linker::LoadFile(const core::Path<char>& path, core::Array<uint8_t>& dataOut)
{
    core::XFileScoped file;
    core::FileFlags mode(core::FileFlag::READ | core::FileFlag::SHARE);

    if (!file.openFile(path, mode)) {
        return false;
    }

    const uint64_t realfileSize = file.remainingBytes();
    const auto fileSize = safe_static_cast<size_t>(realfileSize);

    if (realfileSize > AssetPak::PAK_MAX_ASSET_SIZE) {
        core::HumanSize::Str sizeStr;
        X_ERROR("Linker", "Can't add \"%s\" the asset is too big: %s", path.c_str(), core::HumanSize::toString(sizeStr, realfileSize));
        return false;
    }

    dataOut.resize(fileSize);

    if (file.read(dataOut.data(), dataOut.size()) != fileSize) {
        X_ERROR("Linker", "Failed to read data for\"%s\"", path.c_str());
        return false;
    }

    return true;
}

X_NAMESPACE_END
