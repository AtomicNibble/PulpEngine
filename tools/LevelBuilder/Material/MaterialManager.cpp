#include "stdafx.h"
#include "MaterialManager.h"

#include <../AssetDB/AssetDB.h>

X_NAMESPACE_BEGIN(level)

MatManager::MatManager(assetDb::AssetDB& db, core::MemoryArenaBase* arena) :
    arena_(arena),
    db_(db),
    materials_(arena, sizeof(MaterialResource), core::Max<size_t>(8u, X_ALIGN_OF(MaterialResource)), "MaterialPool"),
    nameOverRide_(arena, 16),
    pDefaultMtl_(nullptr)
{
}

MatManager::~MatManager()
{
}

bool MatManager::Init(void)
{
    X_ASSERT_NOT_NULL(gEnv);

    nameOverRide_.emplace(core::string("caulk"), core::string("tool/editor/caulk"));
    nameOverRide_.emplace(core::string("portal"), core::string("tool/editor/portal"));
    nameOverRide_.emplace(core::string("portal_nodraw"), core::string("tool/editor/portal_nodraw"));
    nameOverRide_.emplace(core::string("berlin_wall_concrete_block"), core::string("floor/concrete/sidewalk"));

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
        pDefaultMtl_ = materials_.createAsset(core::string_view(engine::MTL_DEFAULT_NAME), 
            core::string_view(engine::MTL_DEFAULT_NAME),
            arena_
        );

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
    core::ScopedLock<MaterialContainer::ThreadPolicy> lock(materials_.getThreadPolicy());

    for (auto& m : materials_) {
        as.add(assetDb::AssetType::MATERIAL, m.second->getName());
    }
}

engine::Material* MatManager::loadMaterial(core::string_view name)
{
    X_ASSERT(core::strUtil::FileExtension(name) == nullptr, "Extension not allowed")();

    auto it = nameOverRide_.find(name);
    if (it != nameOverRide_.end()) {
        name = core::string_view(it->second.data(), it->second.length());
    }

    core::ScopedLock<MaterialContainer::ThreadPolicy> lock(materials_.getThreadPolicy());

    MaterialResource* pMatRes = materials_.findAsset(name);
    if (pMatRes) {
        pMatRes->addReference();
        return pMatRes;
    }

    // TODO: perf - maybe skip this string creation.
    core::string nameStr(name.data(), name.length());

    pMatRes = materials_.createAsset(name, name, arena_);
    if (loadMatFromFile(*pMatRes, nameStr)) {
        return pMatRes;
    }

    X_ERROR("MatMan", "Failed to load material: %s", nameStr);

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
    if (!file.openFile(path, core::FileFlag::READ | core::FileFlag::SHARE)) {
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


X_NAMESPACE_END