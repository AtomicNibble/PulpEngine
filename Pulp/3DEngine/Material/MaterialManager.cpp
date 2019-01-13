#include "stdafx.h"
#include "IMaterial.h"
#include "MaterialManager.h"

#include <IFileSys.h>
#include <IShader.h>
#include <IRender.h>
#include <IConsole.h>
#include <ITexture.h>
#include <IVideo.h>

#include <Threading\JobSystem2.h>
#include <Time\StopWatch.h>
#include <Assets\AssetLoader.h>

#include "TechDefStateManager.h"
#include "Drawing\VariableStateManager.h"
#include "Drawing\CBufferManager.h"
#include "Texture\Texture.h"
#include "Texture\TextureManager.h"

#include <CBuffer.h>
#include <Sampler.h>
#include <Texture.h>

X_NAMESPACE_BEGIN(engine)

using namespace render::shader;

XMaterialManager::XMaterialManager(core::MemoryArenaBase* arena, VariableStateManager& vsMan, CBufferManager& cBufMan) :
    arena_(arena),
    blockArena_(arena),
    pAssetLoader_(nullptr),
    pTechDefMan_(nullptr),
    cBufMan_(cBufMan),
    vsMan_(vsMan),
    materials_(arena, sizeof(MaterialResource), core::Max<size_t>(8u, X_ALIGN_OF(MaterialResource)), "MaterialPool"),
    pDefaultMtl_(nullptr),
    failedLoads_(arena)
{
    pTechDefMan_ = X_NEW(TechDefStateManager, arena, "TechDefStateManager")(arena);
}

XMaterialManager::~XMaterialManager()
{
    if (pTechDefMan_) {
        X_DELETE(pTechDefMan_, arena_);
    }
}

void XMaterialManager::registerCmds(void)
{
    ADD_COMMAND_MEMBER("listMaterials", this, XMaterialManager, &XMaterialManager::Cmd_ListMaterials,
        core::VarFlag::SYSTEM, "List all the loaded materials");
}

void XMaterialManager::registerVars(void)
{
    vars_.registerVars();
}

bool XMaterialManager::init(void)
{
    X_ASSERT_NOT_NULL(gEnv);

    pAssetLoader_ = gEnv->pCore->GetAssetLoader();
    pAssetLoader_->registerAssetType(assetDb::AssetType::MATERIAL, this, MTL_B_FILE_EXTENSION);

    if (!initDefaults()) {
        return false;
    }

    return true;
}

void XMaterialManager::shutDown(void)
{
    X_LOG0("Material", "Shutting Down");

    if (pDefaultMtl_) {
        releaseMaterial(pDefaultMtl_);
        pDefaultMtl_ = nullptr;
    }

    freeDangling();

    if (pTechDefMan_) {
        pTechDefMan_->shutDown();
    }
}

bool XMaterialManager::asyncInitFinalize(void)
{
    if (!pDefaultMtl_) {
        X_ERROR("Material", "Default Material is not valid");
        return false;
    }

    if (!waitForLoad(pDefaultMtl_)) {
        X_ERROR("Material", "Failed to load default Material");
        return false;
    }

    pDefaultMtl_->setFlags(pDefaultMtl_->getFlags() | MaterialFlag::DEFAULT);

    // anything that failed to load, while default material was loading
    // assing default to it now.
    core::CriticalSection::ScopedLock lock(failedLoadLock_);

    for (auto* pMat : failedLoads_) {
        X_ASSERT(pMat->getStatus() == core::LoadStatus::Error, "Unexpected status")();
        pMat->assignProps(*pDefaultMtl_);
    }

    failedLoads_.free();
    return true;
}

// IMaterialManager

Material* XMaterialManager::findMaterial(core::string_view name) const
{
    core::ScopedLock<MaterialContainer::ThreadPolicy> lock(materials_.getThreadPolicy());

    Material* pMtl = materials_.findAsset(name);
    if (pMtl) {
        return pMtl;
    }

    X_WARNING("Material", "Failed to find material: \"%*.s\"", name.length(), name.data());
    return nullptr;
}

Material* XMaterialManager::loadMaterial(core::string_view name)
{
    X_ASSERT(core::strUtil::FileExtension(name) == nullptr, "Extension not allowed")();

    MaterialResource* pMatRes = nullptr;
    {
        core::ScopedLock<MaterialContainer::ThreadPolicy> lock(materials_.getThreadPolicy());

        pMatRes = materials_.findAsset(name);
        if (pMatRes) {
            // inc ref count.
            pMatRes->addReference();
            return pMatRes;
        }

        pMatRes = materials_.createAsset(name, name, arena_);
    }

    // add to list of Materials that need loading.
    addLoadRequest(pMatRes);

    return pMatRes;
}

void XMaterialManager::releaseMaterial(Material* pMat)
{
    MaterialResource* pMatRes = reinterpret_cast<MaterialResource*>(pMat);
    if (pMatRes->removeReference() == 0) {
        releaseResources(pMatRes);

        materials_.releaseAsset(pMatRes);
    }
}

bool XMaterialManager::initDefaults(void)
{
    if (pDefaultMtl_ == nullptr) {
        pDefaultMtl_ = loadMaterial(core::string_view(MTL_DEFAULT_NAME));
        if (!pDefaultMtl_) {
            X_ERROR("Material", "Failed to create default material");
            return false;
        }
    }

    return true;
}

void XMaterialManager::freeDangling(void)
{
    {
        core::ScopedLock<AssetContainer::ThreadPolicy> lock(materials_.getThreadPolicy());

        for (const auto& m : materials_) {
            auto* pMatRes = m.second;
            const auto& name = pMatRes->getName();

            X_WARNING("Material", "\"%s\" was not deleted. refs: %" PRIi32, name.c_str(), pMatRes->getRefCount());
            releaseResources(pMatRes);
        }
    }

    materials_.free();
}

void XMaterialManager::releaseResources(Material* pMat)
{
    // when we release the material we need to clean up somethings.
    X_UNUSED(pMat);
}

void XMaterialManager::listMaterials(const char* pSearchPatten) const
{
    core::ScopedLock<MaterialContainer::ThreadPolicy> lock(materials_.getThreadPolicy());

    core::Array<MaterialResource*> sorted_mats(arena_);
    sorted_mats.setGranularity(materials_.size());

    for (const auto& mat : materials_) {
        if (!pSearchPatten || core::strUtil::WildCompare(pSearchPatten, mat.second->getName())) {
            sorted_mats.push_back(mat.second);
        }
    }

    std::sort(sorted_mats.begin(), sorted_mats.end(), [](MaterialResource* a, MaterialResource* b) {
        const auto& nameA = a->getName();
        const auto& nameB = b->getName();
        return nameA.compareInt(nameB) < 0;
    });

    X_LOG0("Material", "------------ ^8Materials(%" PRIuS ")^7 ------------", sorted_mats.size());

    for (const auto* pMat : sorted_mats) {
        X_LOG0("Material", "^2\"%s\"^7 refs: %" PRIi32, pMat->getName().c_str(), pMat->getRefCount());
    }

    X_LOG0("Material", "------------ ^8Materials End^7 -----------");
}

bool XMaterialManager::waitForLoad(core::AssetBase* pMaterial)
{
    X_ASSERT(pMaterial->getType() == assetDb::AssetType::MATERIAL, "Invalid asset passed")();

    if (pMaterial->isLoaded()) {
        return true;
    }

    return waitForLoad(static_cast<Material*>(pMaterial));
}

bool XMaterialManager::waitForLoad(Material* pMaterial)
{
    if (pMaterial->getStatus() == core::LoadStatus::Complete) {
        return true;
    }

    return pAssetLoader_->waitForLoad(pMaterial);
}


void XMaterialManager::addLoadRequest(MaterialResource* pMaterial)
{
    pAssetLoader_->addLoadRequest(pMaterial);
}

void XMaterialManager::onLoadRequestFail(core::AssetBase* pAsset)
{
    auto* pMaterial = static_cast<Material*>(pAsset);

    if (pDefaultMtl_ != pMaterial) {
        // what if default Material not loaded :| ?
        if (!pDefaultMtl_->isLoaded()) {
            // we can't wait for fucking IO load, since this is called by the IO thread.
            // if it's not loaded it will never load.
            core::CriticalSection::ScopedLock lock(failedLoadLock_);

            pMaterial->setStatus(core::LoadStatus::Error);
            failedLoads_.push_back(pMaterial);
        }
        else {
            pMaterial->assignProps(*pDefaultMtl_);
        }
    }
    else {
        pMaterial->setStatus(core::LoadStatus::Error);
    }
}

bool XMaterialManager::processData(core::AssetBase* pAsset, core::UniquePointer<char[]> data, uint32_t dataSize)
{
    auto* pMaterial = static_cast<Material*>(pAsset);

    core::XFileFixedBuf file(data.ptr(), data.ptr() + dataSize);

    return processData(pMaterial, &file);
}

bool XMaterialManager::processData(Material* pMaterial, core::XFile* pFile)
{
    MaterialHeader hdr;
    if (pFile->readObj(hdr) != sizeof(hdr)) {
        return false;
    }

    if (!hdr.isValid()) {
        X_ERROR("Material", "Header is invalid: \"%s\"", pMaterial->getName().c_str());
        return false;
    }

    // we need to poke the goat.
    // so now materials store the cat and type which we can use to get techdef info.
    core::string catType;
    pFile->readString(catType);

    Material::SamplerArr samplers(arena_, hdr.numSamplers);
    Material::ParamArr params(arena_, hdr.numParams);
    Material::TextureArr textures(arena_, hdr.numTextures);

    // now samplers.
    for (uint8_t i = 0; i < hdr.numSamplers; i++) {
        MaterialSampler& sampler = samplers[i];

        pFile->readObj(sampler.sate);
        pFile->readString(sampler.name);
    }

    // now params.
    for (uint8_t i = 0; i < hdr.numParams; i++) {
        MaterialParam& param = params[i];
        pFile->readObj(param.value);
        pFile->readObj(param.type);
        pFile->readString(param.name);
    }

    // now textures.
    for (uint8_t i = 0; i < hdr.numTextures; i++) {
        MaterialTexture& tex = textures[i];
        pFile->readObj(tex.texSlot);
        pFile->readString(tex.name);
        pFile->readString(tex.val);
    }

#if X_DEBUG
    const auto left = pFile->remainingBytes();
    X_WARNING_IF(left > 0, "Material", "potential read fail, bytes left in file: %" PRIu64, left);
#endif // !X_DEBUG

    TechDefState* pTechDefState = pTechDefMan_->getTechDefState(hdr.cat, catType);
    if (!pTechDefState) {
        X_ERROR("Material", "Failed to get techdefstate");
        return false;
    }

    // we want to load all the textures now.
    // so that we are not 'creating' refrences for every tech.
    texture::TextureFlags texFlags = texture::TextureFlags();

    for (auto& tex : textures) {
        if (tex.val.isEmpty()) {
            X_ERROR("Material", "missing texture value");
            return false;
        }

        auto* pTexture = gEngEnv.pTextureMan_->loadTexture(core::string_view(tex.val.data(), tex.val.length()), texFlags);
        tex.pTexture = pTexture;
    }

    pMaterial->assignProps(hdr);
    pMaterial->setTechDefState(pTechDefState);
    pMaterial->setParams(std::move(params));
    pMaterial->setSamplers(std::move(samplers));
    pMaterial->setTextures(std::move(textures));
    pMaterial->setStatus(core::LoadStatus::Complete);
    return true;
}

Material::Tech* XMaterialManager::getTechForMaterial(Material* pMat, core::StrHash techNameHash,
    render::shader::VertexFormat::Enum vrtFmt, PermatationFlags permFlags)
{
    X_ASSERT_NOT_NULL(pMat);

    if (!waitForLoad(pMat)) {
        return nullptr;
    }

    if (pMat->getTextures().isNotEmpty()) {
        permFlags.Set(render::shader::Permatation::Textured);
    }

    // the material holds all it's techs like a cache, but when it don't have one we must create it.
    auto* pTech = pMat->getTech(techNameHash, vrtFmt, permFlags);
    if (pTech) {
        return pTech;
    }

    if (pMat->isDefault()) {
        // if this is not real material and we want to return default material techs.
        pTech = pDefaultMtl_->getTech(techNameHash, vrtFmt, permFlags);
        if (!pTech) {
            pTech = getTechForMaterial_int(pDefaultMtl_, techNameHash, vrtFmt, permFlags);
        }

        return pTech;
    }

    return getTechForMaterial_int(pMat, techNameHash, vrtFmt, permFlags);
}

Material::Tech* XMaterialManager::getTechForMaterial_int(Material* pMat, core::StrHash techNameHash, render::shader::VertexFormat::Enum vrtFmt,
    PermatationFlags permFlags)
{
    X_ASSERT_NOT_NULL(pMat);

    // we must get the techDef so we can select the tech definition.
    TechDefState* pTechDefState = pMat->getTechDefState();

    X_ASSERT_NOT_NULL(pTechDefState);

    // now we have the tech we wnat to create a permatation of it supporting what we want.
    TechDef* pTechDef = pTechDefState->getTech(techNameHash);
    if (!pTechDef) {
        X_ERROR("MatMan", "Tech not found");
        return nullptr;
    }

    // we now have a permatation of the shader that we want.
    // this gives us the pipeline state handle.
    // we just now need to make variable state.
    TechDefPerm* pPerm = pTechDef->getOrCreatePerm(vrtFmt, permFlags);
    if (!pPerm) {
        X_ERROR("MatMan", "Failed to get tech perm");
        return nullptr;
    }

    render::shader::IShaderPermatation* pShaderPerm = pPerm->pShaderPerm;

    // from the shader perm we can see how many const buffers we need to provide.
    const auto& cbLinks = pShaderPerm->getCbufferLinks();
    const auto& buffers = pShaderPerm->getBuffers();
    const auto& permSamplers = pShaderPerm->getSamplers();
    const auto& permTextures = pShaderPerm->getTextures();

    // we need to know how many textures we are going to be sending.
    // it may be less than what the material has.
    // the tech should tell us O_O
    // him -> pTechDef
    const size_t numTex = permTextures.size();
    const size_t numCb = cbLinks.size();
    const size_t numBuffers = buffers.size();
    size_t numSamplers = permSamplers.size();

    if (pTechDef->pTechSecDef_->allSamplersAreStatic()) {
        numSamplers = 0;
    }

    // temp, should move over to using values from perm somepoint.
    X_ASSERT(numTex == (size_t)pPerm->numTextStates, "Mistatch")();
    X_ASSERT(numSamplers == (size_t)pPerm->numSamplers, "Mistatch")();
    X_ASSERT(numCb == (size_t)pPerm->numCbs, "Mistatch")();
    X_ASSERT(numBuffers == (size_t)pPerm->numBuffers, "Mistatch")();

    render::Commands::ResourceStateBase* pVariableState = vsMan_.createVariableState(
        numTex,
        numSamplers,
        numCb,
        numBuffers);

    // we should create the const buffers we need and set them in the variable state.
#if X_ENABLE_ASSERTIONS
    {
        auto* pCBHandles = pVariableState->getCBs();
        auto* pBuffers = pVariableState->getBuffers();

        for (size_t i = 0; i < numCb; i++) {
            pCBHandles[i] = render::INVALID_BUF_HANLDE;
        }
        for (size_t i = 0; i < numBuffers; i++) {
            pBuffers[i].buf = render::INVALID_BUF_HANLDE;
        }
    }
#endif // !X_ENABLE_ASSERTIONS

    {
        // need to map material textures to perm textures.
        // some textures have default values.
        // but we have the compiler set them, so always provided in material.
        // but some textures are set by code.
        auto* pTexStates = pVariableState->getTexStates();
        const auto& matTextures = pMat->getTextures();

        auto* pTexMan = gEngEnv.pTextureMan_;
        auto* pDefaultTex = pTexMan->getDefault(render::TextureSlot::DIFFUSE);

        for (size_t i = 0; i < numTex; i++) {
            auto& texState = pTexStates[i];
            const auto& permTexture = permTextures[i];

            // find texture that matches from material.
            size_t j;
            for (j = 0; j < matTextures.size(); j++) {
                auto& t = matTextures[j];
                if (t.name == permTexture.getName()) {
                    // create the texture instance.
                    // might get default back, etc..
                    texState.textureId = t.pTexture->getDeviceID();

                    X_LOG0("Test", "pVariable: %p texId: %i", pVariableState, texState.textureId);
                    break;
                }
            }

            if (pTechDef->getNumAliases()) {
                const auto& aliases = pTechDef->getAliases();
                for (const auto& alias : aliases) {
                    // find a alias that points to the persm resource.
                    if (alias.resourceName == permTexture.getName()) {

                        // okay so now we know the name of the material sampler that we want.
                        for (j = 0; j < matTextures.size(); j++) {
                            auto& t = matTextures[j];
                            if (t.name == alias.name) {
                                texState.textureId = t.pTexture->getDeviceID();
                                break;
                            }
                        }
                    }
                }
            }

            // find one?
            if (j == matTextures.size()) {
                X_ERROR("Material", "Failed to find texture values for perm texture: \"%s\" using default", permTexture.getName().c_str());

                // really we should know what type of texture would be set
                // eg TextureSlot::Enum
                // this way we can return defaults that would actually not look retarded.

                texState.textureId = pDefaultTex->getDeviceID();
            }

            //		texSet:;
        }
    }

    {
        // so we need to map material samplers to perm samplers
        auto* pSamplers = pVariableState->getSamplers();
        const auto& matSamplers = pMat->getSamplers();

        for (size_t i = 0; i < numSamplers; i++) {
            auto& sampler = pSamplers[i];
            const auto& permSampler = permSamplers[i];

            // find a sampler that matches from material
            size_t j;
            for (j = 0; j < matSamplers.size(); j++) {
                auto& s = matSamplers[j];
                if (s.name == permSampler.getName()) {
                    sampler = s.sate;
                    break;
                }
            }

            // do we want to lookup aliases even if we matched?
            if (pTechDef->getNumAliases()) {
                const auto& aliases = pTechDef->getAliases();
                for (const auto& alias : aliases) {
                    // find a alias that points to the persm resource.
                    if (alias.resourceName == permSampler.getName()) {
                        // okay so now we know the name of the material sampler that we want.
                        for (j = 0; j < matSamplers.size(); j++) {
                            auto& s = matSamplers[j];
                            if (s.name == alias.name) {
                                sampler = s.sate;
                                break;
                            }
                        }
                    }
                }
            }

            if (j == matSamplers.size()) {
                X_ERROR("Material", "Failed to find sampler values for perm sampler: \"%s\" using defaults", permSampler.getName().c_str());
                sampler.filter = render::FilterType::LINEAR_MIP_LINEAR;
                sampler.repeat = render::TexRepeat::TILE_BOTH;
            }
        }
    }

    // we now have a material tech that contains the pipeline state needed and what variable state also needs to be set
    // we just add this to the materials local store, so it don't have to ask us for this next time.
    // so once everything has it's state to render anything we just have to check
    Material::Tech matTech(arena_);
    matTech.hashVal = techNameHash;
    matTech.pPerm = pPerm;
    matTech.pVariableState = pVariableState;

    // now we scan the cbuffers and try to match any material params to cbuffer params (todo: take into account techDef aliases).
    {
        auto* pCBHandles = pVariableState->getCBs();

        matTech.cbs.reserve(cbLinks.size());
        for (auto& cb : cbLinks) {
            matTech.cbs.push_back(cb.pCBufer);
        }

        const auto& matParams = pMat->getParams();

        if (matParams.isNotEmpty()) {
            auto& paramLinks = matTech.paramLinks;

            // a list of all the cb's that contain material params.
            core::FixedArray<int32_t, render::shader::MAX_SHADER_CB_PER_PERM> materialCBIdxs;

            for (size_t i = 0; i < matParams.size(); i++) {
                auto& param = matParams[i];
                auto& name = param.name;
                core::StrHash nameHash(name.begin(), name.end());

                for (size_t j = 0; j < cbLinks.size(); j++) {
                    auto& cbLink = cbLinks[j];
                    auto& cb = *cbLink.pCBufer;

                    for (int32_t p = 0; p < cb.getParamCount(); p++) {
                        auto& cbParam = cb[p];

                        if (cbParam.getNameHash() == nameHash && cbParam.getName() == name) {
                            ParamLink link;
                            link.paramIdx = safe_static_cast<int32_t>(i);
                            link.cbIdx = safe_static_cast<int32_t>(j);
                            link.cbParamIdx = safe_static_cast<int32_t>(p);

                            paramLinks.push_back(link);

                            if (std::find(materialCBIdxs.begin(), materialCBIdxs.end(), link.cbIdx) == materialCBIdxs.end()) {
                                materialCBIdxs.append(link.cbIdx);
                            }
                        }
                    }
                }
            }

            if (paramLinks.isNotEmpty()) {
                X_ASSERT(materialCBIdxs.isNotEmpty(), "Must have atleast one material cb idx, if we have param links")();

                // sort them so in cb order then param order.
                std::sort(paramLinks.begin(), paramLinks.end(), [](const ParamLink& lhs, const ParamLink& rhs) -> bool {
                    if (lhs.cbIdx != rhs.cbIdx) {
                        return lhs.cbIdx < rhs.cbIdx;
                    }
                    return lhs.paramIdx < rhs.paramIdx;
                });

                // reserver so the memory address don't change when appending as we keep pointers.
                matTech.materialCbs.setGranularity(materialCBIdxs.size());
                matTech.materialCbs.reserve(materialCBIdxs.size());

                int32_t currentCbIdx = -1;
                render::shader::XCBuffer matCb(arena_);

                auto addCBToMatTech = [&](int32_t cbIdx, render::shader::XCBuffer& matCb) {
                    matCb.postParamModify();
                    X_ASSERT(matCb.containsUpdateFreqs(render::shader::UpdateFreq::MATERIAL), "Should contain per material params")();
                    matTech.materialCbs.append(std::move(matCb));

                    // change the pointer to mat instance, instead of the instance from the shader perm which is shared.
                    matTech.cbs[cbIdx] = &matTech.materialCbs.back();
                };

                for (auto& link : paramLinks) {
                    if (currentCbIdx != link.cbIdx) {
                        if (currentCbIdx != -1) {
                            addCBToMatTech(link.cbIdx, matCb);
                        }

                        currentCbIdx = link.cbIdx;
                        const auto& cbLink = cbLinks[link.cbIdx];
                        const auto& cb = *cbLink.pCBufer;

                        matCb = cb; // make a copy.
                    }

                    // change the update freq.
                    // potentially and param that's unkown could just be default marked as material param.
                    // would make some things more simple.
                    auto& cpuData = matCb.getCpuData();
                    auto& cbParam = matCb[link.cbParamIdx];
                    const auto& matParam = matParams[link.paramIdx];

                    cbParam.setUpdateRate(render::shader::UpdateFreq::MATERIAL);

                    // copy the material params value into the
                    // always vec4? humm.
                    X_ASSERT(cpuData.size() >= (cbParam.getBindOffset() + sizeof(matParam.value)), "Overflow when writing mat param value to cbuffer")();
                    std::memcpy(&cpuData[cbParam.getBindOffset()], &matParam.value, sizeof(matParam.value));
                }

                // optermisation:
                // might be better to not store these material cb instance for each tech and instead share them within a material
                // so if a material has multiple techs that have identical material cbuffers they could share them.
                // since the material params will be the same for all techs.
                addCBToMatTech(currentCbIdx, matCb);
            }
        }

        // the 'cbuffers' arr can contain a mix of cbuffer's pointers that can point at material instances or ones from the shader perm.
        X_ASSERT(cbLinks.size() == matTech.cbs.size(), "Links and cbuffer list should be same size")(cbLinks.size(), matTech.cbs.size());
        for (size_t i = 0; i < matTech.cbs.size(); i++) {
            auto* pCB = matTech.cbs[i];
            X_ASSERT_NOT_NULL(pCB);
            pCBHandles[i] = cBufMan_.createCBuffer(*pCB);
        }
    }

#if X_ENABLE_ASSERTIONS
    {
        auto* pCBHandles = pVariableState->getCBs();

        for (size_t i = 0; i < numCb; i++) {
            X_ASSERT(pCBHandles[i] != render::INVALID_BUF_HANLDE, "Cbuffer handle is invalid")();
        }
    }
#endif // !X_ENABLE_ASSERTIONS

    pMat->addTech(std::move(matTech));

    auto* pTech = pMat->getTech(techNameHash, vrtFmt, permFlags);

    return pTech;
}

bool XMaterialManager::setTextureID(Material* pMat, Material::Tech* pTech, core::StrHash texNameHash, texture::TexID id)
{
    TechDefState* pTechDefState = pMat->getTechDefState();

    TechDef* pTechDef = pTechDefState->getTech(core::StrHash(pTech->hashVal));

    if (pTechDef->getNumAliases() < 1) {
        return false;
    }

    auto& als = pTechDef->getAliases();

    for (size_t i = 0; i < als.size(); i++) {
        const auto& alias = als[i];
        if (alias.nameHash == texNameHash) {
            auto* pTextureStates = pTech->pVariableState->getTexStates();
            pTextureStates[i].textureId = id;
            return true;
        }
    }

    return false;
}

bool XMaterialManager::setRegisters(MaterialTech* pTech, const RegisterCtx& regs)
{
    setRegisters(pTech->pPerm, pTech->pVariableState, regs);

    return true;
}

void XMaterialManager::initStateFromRegisters(TechDefPerm* pTech,
    render::Commands::ResourceStateBase* pResourceState, const RegisterCtx& regs)
{
    // set the sizes like a hoe.
    pResourceState->setSizes(
        pTech->numTextStates,
        pTech->numSamplers,
        pTech->numCbs,
        pTech->numBuffers);

    setRegisters(pTech, pResourceState, regs);
}

X_INLINE void XMaterialManager::setRegisters(TechDefPerm* pTech,
    render::Commands::ResourceStateBase* pResourceState, const RegisterCtx& regs)
{
    TechDef* pTechDef = pTech->pTechDef;

    // set the fooking state yo!
    const auto& permTextures = pTech->pShaderPerm->getTextures();

    for (auto& cb : pTechDef->codeBinds_) {
        uint32_t val = regs.regs[cb.slot];

        for (size_t i = 0; i < permTextures.size(); i++) {
            auto& p = permTextures[i];

            if (p.getName() == cb.resourceName) {
                X_ASSERT(i < static_cast<size_t>(pResourceState->getNumTextStates()), "Resouce index out of bounds")(i, pResourceState->getNumTextStates());
                pResourceState->getTexStates()[i].textureId = val;
                break;
            }
        }
    }
}

TechDefPerm* XMaterialManager::getCodeTech(const core::string& name, core::StrHash techName,
    render::shader::VertexFormat::Enum vertFmt, PermatationFlags permFlags)
{
    TechDefState* pTechDefState = pTechDefMan_->getTechDefState(MaterialCat::CODE, name);
    if (!pTechDefState) {
        X_ERROR("MatMan", "Failed to get techdefstate");
        return nullptr;
    }

    TechDef* pTechDef = pTechDefState->getTech(techName);
    if (!pTechDef) {
        X_ERROR("MatMan", "Failed to get TechDef");
        return nullptr;
    }

    TechDefPerm* pPerm = pTechDef->getOrCreatePerm(vertFmt, permFlags);
    if (!pPerm) {
        X_ERROR("MatMan", "Failed to get tech perm");
        return nullptr;
    }

    return pPerm;
}

bool XMaterialManager::onFileChanged(const core::AssetName& assetName, const core::string& name)
{
    X_UNUSED(assetName, name);

    return true;
}

void XMaterialManager::Cmd_ListMaterials(core::IConsoleCmdArgs* pCmd)
{
    // optional search criteria
    const char* pSearchPatten = nullptr;

    if (pCmd->GetArgCount() >= 2) {
        pSearchPatten = pCmd->GetArg(1);
    }

    listMaterials(pSearchPatten);
}

X_NAMESPACE_END
