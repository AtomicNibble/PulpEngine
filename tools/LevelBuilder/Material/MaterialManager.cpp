#include "stdafx.h"
#include "MaterialManager.h"

#include <../AssetDB/AssetDB.h>

X_NAMESPACE_BEGIN(level)

MatManager::MatManager(assetDb::AssetDB& db, core::MemoryArenaBase* arena) :
    arena_(arena),
    db_(db),
    materials_(arena, sizeof(MaterialResource), core::Max<size_t>(8u, X_ALIGN_OF(MaterialResource)), "MaterialPool"),
    nameOverRide_(arena, 64),
    pDefaultMtl_(nullptr)
{
}

MatManager::~MatManager()
{
}

bool MatManager::Init(void)
{
    X_ASSERT_NOT_NULL(gEnv);

    nameOverRide_.insert(std::make_pair(core::string("portal"), core::string("tool/editor/portal")));
    nameOverRide_.insert(std::make_pair(core::string("portal_nodraw"), core::string("tool/editor/portal_nodraw")));
    nameOverRide_.insert(std::make_pair(core::string("berlin_wall_concrete_block"), core::string("floor/concrete/sidewalk")));

    return loadDefaultMaterial();
}

void MatManager::ShutDown(void)
{
    // clear everything up.
    releaseMaterial(pDefaultMtl_);

    freeDanglingMaterials();
}

bool MatManager::loadDefaultMaterial(void)
{
    if (!pDefaultMtl_) {
        pDefaultMtl_ = createMaterial_Internal(core::string(engine::MTL_DEFAULT_NAME));

        // it's default :|
        pDefaultMtl_->setFlags(engine::MaterialFlag::DEFAULT | engine::MaterialFlag::STRUCTURAL);
        pDefaultMtl_->setCoverage(engine::MaterialCoverage::OPAQUE);
    }

    return true;
}

void MatManager::freeDanglingMaterials(void)
{
    auto it = materials_.begin();
    for (; it != materials_.end(); ++it) {
        auto matRes = it->second;
        X_WARNING("MtlMan", "\"%s\" was not deleted. refs: %" PRIi32, matRes->getName(), matRes->getRefCount());
    }

    materials_.free();
}

engine::Material* MatManager::getDefaultMaterial(void) const
{
    return pDefaultMtl_->instance();
}

void MatManager::addAssets(linker::AssetList& as) const
{
    for (auto& m : materials_) {
        as.add(assetDb::AssetType::MATERIAL, m.first);
    }
}

engine::Material* MatManager::loadMaterial(const char* pMtlName)
{
    X_ASSERT_NOT_NULL(pMtlName);
    X_ASSERT(core::strUtil::FileExtension(pMtlName) == nullptr, "Extension not allowed")(pMtlName); 

    core::string name(pMtlName);

    auto it = nameOverRide_.find(name);
    if (it != nameOverRide_.end()) {
        name = it->second;
    }

    if (name != "tool/editor/portal" && name != "tool/editor/portal_nodraw") {
        name = "floor/concrete/sidewalk";
    }

    // try find it.
    MaterialResource* pMatRes = findMaterial_Internal(name);
    if (pMatRes) {
        pMatRes->addReference();
        return pMatRes;
    }

    // create a new material.
    pMatRes = createMaterial_Internal(name);
    if (loadMatFromFile(*pMatRes, name)) {
        return pMatRes;
    }

    X_ERROR("MatMan", "Failed to load material: %s", pMtlName);

    pMatRes->assignProps(*pDefaultMtl_);
    return pMatRes;
}

void MatManager::releaseMaterial(engine::Material* pMat)
{
    X_ASSERT_NOT_NULL(pMat);

    MaterialResource* pMatRes = reinterpret_cast<MaterialResource*>(pMat);
    if (pMatRes->removeReference() == 0) {
        materials_.releaseAsset(pMatRes);
    }
}

bool MatManager::loadMatFromFile(MaterialResource& mat, const core::string& name)
{
    core::Path<char> path;
    if (!getMatPath(name, path)) {
        return false;
    }

    core::XFileScoped file;
    if (!file.openFile(path.c_str(), core::fileMode::READ | core::fileMode::SHARE)) {
        return false;
    }

    engine::MaterialHeader hdr;
    if (file.readObj(hdr) != sizeof(hdr)) {
        return false;
    }

    if (!hdr.isValid()) {
        return false;
    }

    mat.assignProps(hdr);
    mat.setStatus(core::LoadStatus::Complete);
    return true;
}

bool MatManager::getMatPath(const core::string& name, core::Path<char>& path)
{
    assetDb::AssetId assetId = assetDb::INVALID_ASSET_ID;
    assetDb::AssetDB::ModId modId;
    if (!db_.AssetExists(assetDb::AssetType::MATERIAL, name, &assetId, &modId)) {
        X_ERROR("MatMan", "Mat does not exists: \"%s\"", name.c_str());
        return false;
    }

    if (!db_.GetOutputPathForAsset(modId, assetDb::AssetType::MATERIAL, name, path)) {
        X_ERROR("ModelCache", "Failed to asset path");
        return false;
    }

    return true;
}

MatManager::MaterialResource* MatManager::createMaterial_Internal(core::string& name)
{
    // internal create expects you to know no duplicates
    X_ASSERT(findMaterial_Internal(name) == nullptr, "Creating a material that already exsists")(); 

    auto pMatRes = materials_.createAsset(name, name, arena_);

    return pMatRes;
}

MatManager::MaterialResource* MatManager::findMaterial_Internal(const core::string& name) const
{
    return materials_.findAsset(name);
}

X_NAMESPACE_END