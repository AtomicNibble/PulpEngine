#pragma once

X_NAMESPACE_DECLARE(assetDb, class AssetDB)


X_NAMESPACE_BEGIN(level)

class ModelCache
{
    typedef core::HashMap<core::string, AABB> ModelMap;

public:
    ModelCache(assetDb::AssetDB& db, core::MemoryArenaBase* arena);
    ~ModelCache();

    bool getModelAABB(const core::string& name, AABB& bounds);
    bool loadDefaultModel(void);

private:
    bool getModelPath(const core::string& name, core::Path<char>& path);


private:
    core::MemoryArenaBase* arena_;
    assetDb::AssetDB& db_;
    ModelMap models_;

    AABB defaultModelBounds_;
};

X_NAMESPACE_END
