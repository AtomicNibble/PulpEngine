#pragma once

#include <IAsyncLoad.h>

X_NAMESPACE_BEGIN(model)

class XModel;

struct IModelManager : public core::IAssetLoader
{
    using core::IAssetLoader::waitForLoad;

    virtual ~IModelManager() = default;

    // returns null if not found, ref count unaffected
    virtual XModel* findModel(core::AssetID id) const X_ABSTRACT;
    virtual XModel* findModel(core::string_view name) const X_ABSTRACT;
    virtual XModel* loadModel(core::string_view name) X_ABSTRACT;
    virtual XModel* getDefaultModel(void) const X_ABSTRACT;

    virtual void releaseModel(XModel* pModel) X_ABSTRACT;

    virtual bool waitForLoad(XModel* pModel) X_ABSTRACT; // returns true if load succeed.

    X_INLINE bool waitForLoad(core::AssetBase* pModel) X_FINAL;
};

X_INLINE bool IModelManager::waitForLoad(core::AssetBase* pModel)
{
    X_ASSERT(pModel->getType() == assetDb::AssetType::MODEL, "Invalid asset passed")();

    if (pModel->isLoaded()) {
        return true;
    }

    return waitForLoad(static_cast<XModel*>(pModel));
}

X_NAMESPACE_END
