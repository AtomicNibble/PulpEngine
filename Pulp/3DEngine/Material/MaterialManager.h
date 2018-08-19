#pragma once

#ifndef X_3D_MATERIAL_MAN_H_
#define X_3D_MATERIAL_MAN_H_

#include <String\StrRef.h>

#include <Assets\AssertContainer.h>

#include "Vars\MaterialVars.h"

X_NAMESPACE_DECLARE(core,
                    namespace V2 {
                        struct Job;
                        class JobSystem;
                    }

                    struct IConsoleCmdArgs;)

X_NAMESPACE_BEGIN(engine)

namespace techset
{
    class TechSetDefs;

} // namespace techset

class VariableStateManager;
class TechDefStateManager;
class CBufferManager;
class Material;

class XMaterialManager : public IMaterialManager
    , public ICoreEventListener
    , private core::IAssetLoadSink
{
    typedef core::AssetContainer<Material, MTL_MAX_LOADED, core::SingleThreadPolicy> MaterialContainer;
    typedef MaterialContainer::Resource MaterialResource;
    typedef MaterialContainer AssetContainer;

    typedef core::Array<Material*> MaterialArr;

public:
    XMaterialManager(core::MemoryArenaBase* arena, VariableStateManager& vsMan, CBufferManager& cBufMan);
    virtual ~XMaterialManager();

    void registerCmds(void);
    void registerVars(void);

    bool init(void);
    void shutDown(void);

    bool asyncInitFinalize(void);

    // IMaterialManager
    Material* findMaterial(const char* pMtlName) const X_FINAL;
    Material* loadMaterial(const char* pMtlName) X_FINAL;
    Material* getDefaultMaterial(void) const X_FINAL;

    // returns true if load succeed.
    bool waitForLoad(core::AssetBase* pMaterial) X_FINAL;
    bool waitForLoad(Material* pMaterial) X_FINAL;
    void releaseMaterial(Material* pMat);

    Material::Tech* getTechForMaterial(Material* pMat, core::StrHash hash, render::shader::VertexFormat::Enum vrtFmt,
        PermatationFlags permFlags = PermatationFlags()) X_FINAL;
    bool setTextureID(Material* pMat, Material::Tech* pTech, core::StrHash texNameHash, texture::TexID id) X_FINAL;

    bool setRegisters(MaterialTech* pTech, const RegisterCtx& regs) X_FINAL;
    void initStateFromRegisters(TechDefPerm* pTech, render::Commands::ResourceStateBase* pResourceState, const RegisterCtx& regs);

    TechDefPerm* getCodeTech(const core::string& name, core::StrHash techName, render::shader::VertexFormat::Enum,
        PermatationFlags permFlags = PermatationFlags());

    // ~IMaterialManager
    void listMaterials(const char* pSearchPatten = nullptr) const;

private:
    void setRegisters(TechDefPerm* pTech, render::Commands::ResourceStateBase* pResourceState, const RegisterCtx& regs);

    Material::Tech* getTechForMaterial_int(Material* pMat, core::StrHash hash, render::shader::VertexFormat::Enum vrtFmt,
        PermatationFlags permFlags);

private:
    bool initDefaults(void);
    void freeDangling(void);
    void releaseResources(Material* pMat);

    void addLoadRequest(MaterialResource* pMaterial);
    void onLoadRequestFail(core::AssetBase* pAsset) X_FINAL;
    bool processData(core::AssetBase* pAsset, core::UniquePointer<char[]> data, uint32_t dataSize) X_FINAL;
    bool onFileChanged(const core::AssetName& assetName, const core::string& name) X_FINAL;

    bool processData(Material* pMaterial, core::XFile* pFile);

private:
    // ICoreEventListener
    void OnCoreEvent(CoreEvent::Enum event, const CoreEventData& ed) X_FINAL;
    // ~ICoreEventListener

private:
    void Cmd_ListMaterials(core::IConsoleCmdArgs* pCmd);

private:
    core::MemoryArenaBase* arena_;
    core::MemoryArenaBase* blockArena_;

    core::AssetLoader* pAssetLoader_;

    CBufferManager& cBufMan_;
    VariableStateManager& vsMan_;
    TechDefStateManager* pTechDefMan_;

    MaterialVars vars_;
    MaterialContainer materials_;

    Material* pDefaultMtl_;

    core::CriticalSection failedLoadLock_;
    MaterialArr failedLoads_;
};

X_NAMESPACE_END

#include "MaterialManager.inl"

#endif // X_3D_MATERIAL_MAN_H_