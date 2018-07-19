#include "stdafx.h"
#include "ModelCache.h"

#include <../AssetDB/AssetDB.h>


X_NAMESPACE_BEGIN(level)

ModelCache::ModelCache(assetDb::AssetDB& db, core::MemoryArenaBase* arena) :
    arena_(arena),
    db_(db),
    models_(arena, 256)
{
}

ModelCache::~ModelCache()
{
}

bool ModelCache::getModelAABB(const core::string& name, AABB& bounds)
{
    auto it = models_.find(name);
    if (it != models_.end()) {
        bounds = it->second;
        return true;
    }

    core::Path<char> path;
    if (!getModelPath(name, path)) {
        return false;
    }

    if (!model::Util::GetModelAABB(path, bounds)) {
        bounds = defaultModelBounds_;
        // we don't insert in cache otherwise it would return true for the next request.
        return false;
    }

    models_.insert(std::make_pair(name, bounds));
    return true;
}

bool ModelCache::loadDefaultModel(void)
{
    if (!getModelAABB(core::string(model::MODEL_DEFAULT_NAME), defaultModelBounds_)) {
        X_ERROR("Lvl", "Failed to load default model info");
        return false;
    }
    return true;
}

bool ModelCache::getModelPath(const core::string& name, core::Path<char>& path)
{
    assetDb::AssetId assetId = assetDb::INVALID_ASSET_ID;
    assetDb::AssetDB::ModId modId;
    if (!db_.AssetExsists(assetDb::AssetType::MODEL, name, &assetId, &modId)) {
        X_ERROR("ModelCache", "Model does not exists: \"%s\"",  name.c_str());
        return false;
    }

    assetDb::AssetDB::Mod modInfo;
    if (!db_.GetModInfo(modId, modInfo)) {
        X_ERROR("ModelCache", "Failed to get mod info");
        return false;
    }

    assetDb::AssetDB::GetOutputPathForAsset(assetDb::AssetType::MODEL, name, modInfo.outDir, path);
    return true;
}

X_NAMESPACE_END
