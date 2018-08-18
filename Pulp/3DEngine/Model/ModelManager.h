#pragma once

#ifndef X_MODEL_MANAGER_H_
#define X_MODEL_MANAGER_H_

#include <IModel.h>
#include <IModelManager.h>

#include <Assets\AssertContainer.h>
#include <IAsyncLoad.h>

X_NAMESPACE_DECLARE(core,
                    namespace V2 {
                        struct Job;
                        class JobSystem;
                    }

                    struct IConsoleCmdArgs;)

X_NAMESPACE_BEGIN(model)

class RenderModel;

class XModelManager : public IModelManager
    , private core::IAssetLoadSink
{
    typedef core::AssetContainer<RenderModel, MODEL_MAX_LOADED, core::SingleThreadPolicy> ModelContainer;
    typedef ModelContainer::Resource ModelResource;

public:
    XModelManager(core::MemoryArenaBase* arena, core::MemoryArenaBase* blockArena);
    ~XModelManager() X_OVERRIDE;

    void registerCmds(void);
    void registerVars(void);

    bool init(void);
    void shutDown(void);

    bool asyncInitFinalize(void);

    XModel* findModel(const char* pModelName) const X_FINAL;
    XModel* loadModel(const char* pModelName) X_FINAL;
    XModel* getDefaultModel(void) const X_FINAL;

    void releaseModel(XModel* pModel) X_FINAL;

    void listModels(const char* pSearchPatten = nullptr) const;

    bool waitForLoad(XModel* pModel) X_FINAL; // returns true if load succeed.

private:
    bool initDefaults(void);
    void freeDangling(void);
    void releaseResources(XModel* pModel);

    // load / processing
    void addLoadRequest(ModelResource* pModel);
    void onLoadRequestFail(core::AssetBase* pAsset) X_FINAL;
    bool processData(core::AssetBase* pAsset, core::UniquePointer<char[]> data, uint32_t dataSize) X_FINAL;
    bool onFileChanged(const core::AssetName& assetName, const core::string& name) X_FINAL;

private:
    void Cmd_ListModels(core::IConsoleCmdArgs* pCmd);

private:
    core::MemoryArenaBase* arena_;
    core::MemoryArenaBase* blockArena_; // for the model data buffers

    core::AssetLoader* pAssetLoader_;

    RenderModel* pDefaultModel_;
    ModelContainer models_;
};

X_NAMESPACE_END

#endif // !X_MODEL_MANAGER_H_