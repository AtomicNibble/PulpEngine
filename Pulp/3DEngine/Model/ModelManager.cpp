#include <stdafx.h>
#include <Model\ModelManager.h>

#include <Assets\AssetLoader.h>

#include "EngineEnv.h"
#include "Material\MaterialManager.h"
#include "RenderModel.h"

#include <IConsole.h>
#include <IFileSys.h>
#include <Threading\JobSystem2.h>
#include <Time\StopWatch.h>
#include <String\AssetName.h>

X_NAMESPACE_BEGIN(model)

XModelManager::XModelManager(core::MemoryArenaBase* arena, core::MemoryArenaBase* blockArena) :
    arena_(arena),
    blockArena_(blockArena),
    pAssetLoader_(nullptr),
    pDefaultModel_(nullptr),
    models_(arena, sizeof(ModelResource), X_ALIGN_OF(ModelResource), "ModelPool")
{

}

XModelManager::~XModelManager()
{
}

void XModelManager::registerCmds(void)
{
    ADD_COMMAND_MEMBER("listModels", this, XModelManager, &XModelManager::Cmd_ListModels, core::VarFlag::SYSTEM, "List all the loaded models");
}

void XModelManager::registerVars(void)
{
}

bool XModelManager::init(void)
{
    X_ASSERT_NOT_NULL(gEnv);

    pAssetLoader_ = gEnv->pCore->GetAssetLoader();
    pAssetLoader_->registerAssetType(assetDb::AssetType::MODEL, this, MODEL_FILE_EXTENSION);

    if (!initDefaults()) {
        return false;
    }

    return true;
}

void XModelManager::shutDown(void)
{
    X_LOG0("ModelManager", "Shutting Down");

    // default model
    if (pDefaultModel_) {
        releaseModel(pDefaultModel_);
    }

    freeDangling();
}

bool XModelManager::asyncInitFinalize(void)
{
    if (!pDefaultModel_) {
        X_ERROR("ModelManager", "Default model is not valid");
        return false;
    }

    if (!waitForLoad(pDefaultModel_)) {
        X_ERROR("ModelManager", "Failed to load default model");
        return false;
    }

    return true;
}

XModel* XModelManager::findModel(const char* pModelName) const
{
    core::string name(pModelName);
    core::ScopedLock<ModelContainer::ThreadPolicy> lock(models_.getThreadPolicy());

    ModelResource* pModel = models_.findAsset(name);
    if (pModel) {
        return pModel;
    }

    X_WARNING("ModelManager", "Failed to find model: \"%s\"", pModelName);
    return nullptr;
}

XModel* XModelManager::loadModel(const char* pModelName)
{
    X_ASSERT_NOT_NULL(pModelName);
    X_ASSERT(core::strUtil::FileExtension(pModelName) == nullptr, "Extension not allowed")(pModelName);

    core::string name(pModelName);
    core::ScopedLock<ModelContainer::ThreadPolicy> lock(models_.getThreadPolicy());

    ModelResource* pModelRes = models_.findAsset(name);
    if (pModelRes) {
        // inc ref count.
        pModelRes->addReference();
        return pModelRes;
    }

    // we create a model and give it back
    pModelRes = models_.createAsset(name, name);

    // add to list of models that need loading.
    addLoadRequest(pModelRes);

    return pModelRes;
}

XModel* XModelManager::getDefaultModel(void) const
{
    return pDefaultModel_;
}

void XModelManager::releaseModel(XModel* pModel)
{
    ModelResource* pModelRes = static_cast<ModelResource*>(pModel);
    if (pModelRes->removeReference() == 0) {
        releaseResources(pModelRes);

        models_.releaseAsset(pModelRes);
    }
}

bool XModelManager::initDefaults(void)
{
    pDefaultModel_ = static_cast<RenderModel*>(loadModel(MODEL_DEFAULT_NAME));
    if (!pDefaultModel_) {
        X_ERROR("ModelManager", "Failed to create default model");
        return false;
    }

    return true;
}

void XModelManager::freeDangling(void)
{
    {
        core::ScopedLock<ModelContainer::ThreadPolicy> lock(models_.getThreadPolicy());

        // any left?
        for (const auto& m : models_) {
            auto* pModelRes = m.second;
            const auto& name = pModelRes->getName();
            X_WARNING("ModelManager", "\"%s\" was not deleted. refs: %" PRIi32, name.c_str(), pModelRes->getRefCount());

            releaseResources(pModelRes);
        }
    }

    models_.free();
}

void XModelManager::releaseResources(XModel* pModel)
{
    X_UNUSED(pModel);
}

bool XModelManager::waitForLoad(XModel* pModel)
{
    if (pModel->getStatus() == core::LoadStatus::Complete) {
        return true;
    }

    return pAssetLoader_->waitForLoad(pModel);
}

void XModelManager::addLoadRequest(ModelResource* pModel)
{
    pAssetLoader_->addLoadRequest(pModel);
}

void XModelManager::onLoadRequestFail(core::AssetBase* pAsset)
{
    auto* pModel = static_cast<RenderModel*>(pAsset);

    if (pModel != pDefaultModel_) {
        // what if default model not loaded :| ?
        if (!pDefaultModel_->isLoaded()) {
            waitForLoad(pDefaultModel_);
        }

        // only assing if valid.
        if (pDefaultModel_->isLoaded()) {
            pModel->assignDefault(pDefaultModel_);
        }
    }
}

bool XModelManager::processData(core::AssetBase* pAsset, core::UniquePointer<char[]> data, uint32_t dataSize)
{
    auto* pModel = static_cast<RenderModel*>(pAsset);

    return pModel->processData(std::move(data), dataSize, X_ASSERT_NOT_NULL(engine::gEngEnv.pMaterialMan_));
}

void XModelManager::listModels(const char* pSearchPatten) const
{
    core::ScopedLock<ModelContainer::ThreadPolicy> lock(models_.getThreadPolicy());

    core::Array<ModelResource*> sorted_models(g_3dEngineArena);
    sorted_models.setGranularity(models_.size());

    for (const auto& model : models_) {
        auto* pModelRes = model.second;

        if (!pSearchPatten || core::strUtil::WildCompare(pSearchPatten, pModelRes->getName())) {
            sorted_models.push_back(pModelRes);
        }
    }

    std::sort(sorted_models.begin(), sorted_models.end(), [](ModelResource* a, ModelResource* b) {
        const auto& nameA = a->getName();
        const auto& nameB = b->getName();
        return nameA.compareInt(nameB) < 0;
    });

    X_LOG0("Model", "------------ ^8Models(%" PRIuS ")^7 ---------------", sorted_models.size());

    for (const auto* pModel : sorted_models) {
        X_LOG0("Model", "^2%-32s^7 Lods: ^2%i^7 Bones: ^2%i^7 RootBones: ^2%i^7 TotalMesh: ^2%i^7 Phys: ^2%" PRIi8 "^7 Refs: ^2%i",
            pModel->getName(), pModel->getNumLods(), pModel->getNumBones(), pModel->getNumRootBones(),
            pModel->getNumMeshTotal(), pModel->hasPhys(), pModel->getRefCount());
    }

    X_LOG0("Model", "------------ ^8Models End^7 --------------");
}

bool XModelManager::onFileChanged(const core::AssetName& assetName, const core::string& name)
{
    X_UNUSED(assetName);

    core::ScopedLock<ModelContainer::ThreadPolicy> lock(models_.getThreadPolicy());

    auto* pModelRes = models_.findAsset(name);
    if (!pModelRes) {
        X_LOG1("ModelManager", "Not reloading \"%s\" it's not currently used", name.c_str());
        return false;
    }

    X_LOG0("ModelManager", "Reloading: %s", name.c_str());

    pAssetLoader_->reload(pModelRes, core::ReloadFlag::Beginframe);
    return true;
}

void XModelManager::Cmd_ListModels(core::IConsoleCmdArgs* pCmd)
{
    // optional search criteria
    const char* pSearchPatten = nullptr;

    if (pCmd->GetArgCount() >= 2) {
        pSearchPatten = pCmd->GetArg(1);
    }

    listModels(pSearchPatten);
}

X_NAMESPACE_END
