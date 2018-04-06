#include "stdafx.h"
#include "ModelCache.h"

X_NAMESPACE_BEGIN(level)

ModelCache::ModelCache(core::MemoryArenaBase* arena) :
    arena_(arena),
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

    if (!model::Util::GetModelAABB(name, bounds)) {
        bounds = defaultModelBounds_;
        // we don't insert in cache otherwise it would return true for the next request.
        return false;
    }

    models_.insert(std::make_pair(name, bounds));
    return true;
}

bool ModelCache::loadDefaultModel(void)
{
    if (!model::Util::GetModelAABB(core::string(model::MODEL_DEFAULT_NAME), defaultModelBounds_)) {
        X_ERROR("Lvl", "Failed to load default model info");
        return false;
    }
    return true;
}

X_NAMESPACE_END
