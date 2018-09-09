#pragma once

#ifndef X_LVL_BUILDER_MAT_MANAGER_H_
#define X_LVL_BUILDER_MAT_MANAGER_H_

#include <Util\ReferenceCounted.h>
#include <Assets\AssertContainer.h>

#include <../../tools/MaterialLib/MatLib.h>

X_NAMESPACE_DECLARE(assetDb, class AssetDB)

X_NAMESPACE_BEGIN(level)

class MatManager
{
    typedef core::AssetContainer<engine::Material, engine::MTL_MAX_LOADED, core::SingleThreadPolicy> MaterialContainer;
    typedef MaterialContainer::Resource MaterialResource;

    typedef core::HashMap<core::string, core::string> NameOverrideMap;

public:
    MatManager(assetDb::AssetDB& db, core::MemoryArenaBase* arena);
    ~MatManager();

    bool Init(void);
    void ShutDown(void);

    engine::Material* loadMaterial(const char* MtlName);
    void releaseMaterial(engine::Material* pMat);

    engine::Material* getDefaultMaterial(void) const;

    void addAssets(linker::AssetList& as) const;

private:
    bool loadDefaultMaterial(void);
    void freeDanglingMaterials(void);

private:
    bool loadMatFromFile(MaterialResource& mat, const core::string& name);

    bool getMatPath(const core::string& name, core::Path<char>& path);

    // only call if you know don't exsists in map.
    MaterialResource* createMaterial_Internal(core::string& name);
    MaterialResource* findMaterial_Internal(const core::string& name) const;

private:
    core::MemoryArenaBase* arena_;
    assetDb::AssetDB& db_;

    MaterialContainer materials_;
    NameOverrideMap nameOverRide_;

    MaterialResource* pDefaultMtl_;
};

X_NAMESPACE_END

#endif // !X_LVL_BUILDER_MAT_MANAGER_H_