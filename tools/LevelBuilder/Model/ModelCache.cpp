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

void ModelCache::addAssets(linker::AssetList& as) const
{
    for (auto& m : models_) {
        as.add(assetDb::AssetType::MODEL, m.first);
    }
}

bool ModelCache::getModelPath(const core::string& name, core::Path<char>& path)
{
    assetDb::AssetId assetId = assetDb::INVALID_ASSET_ID;
    assetDb::AssetDB::ModId modId;
    if (!db_.AssetExists(assetDb::AssetType::MODEL, name, &assetId, &modId)) {
        X_ERROR("ModelCache", "Model does not exists: \"%s\"",  name.c_str());
        return false;
    }

    if (!db_.GetOutputPathForAsset(modId, assetDb::AssetType::MODEL, name, path)) {
        X_ERROR("ModelCache", "Failed to asset path");
        return false;
    }

    return true;
}

X_NAMESPACE_END
