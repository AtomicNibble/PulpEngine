#include "stdafx.h"
#include "TextureManager.h"
#include "Texture.h"

#include <Threading\JobSystem2.h>
#include <IConsole.h>
#include <ICi.h>
#include <IFileSys.h>
#include <ICompression.h>
#include <Assets\AssetLoader.h>

#include <Compression\CompressorAlloc.h>
#include <String\AssetName.h>

X_NAMESPACE_BEGIN(engine)

namespace
{
    struct CIFileJobData
    {
        Texture* pTexture;
        const uint8_t* pData;
        size_t length;
    };

} // namespace

TextureManager::TextureManager(core::MemoryArenaBase* arena) :
    arena_(arena),
    pAssetLoader_(nullptr),
    textures_(arena, sizeof(TextureResource), core::Max(64_sz, X_ALIGN_OF(TextureResource)), "TexturePool"),
    pCILoader_(nullptr),

    blockAlloc_(),
    blockArena_(&blockAlloc_, "TextureBlockAlloc"),
    streamQueue_(arena),
    currentDeviceTexId_(0),
    pTexDefault_(nullptr),
    pTexDefaultBump_(nullptr)
{
    defaultLookup_.fill(nullptr);
}

TextureManager::~TextureManager()
{
}

void TextureManager::registerCmds(void)
{
    ADD_COMMAND_MEMBER("listImages", this, TextureManager, &TextureManager::Cmd_ListTextures, core::VarFlag::SYSTEM,
        "List all the textures");
}

void TextureManager::registerVars(void)
{
    vars_.registerVars();
}

bool TextureManager::init(void)
{
    X_ASSERT_NOT_NULL(gEnv);
    X_ASSERT_NOT_NULL(gEnv->pConsole);
    X_LOG1("TextureManager", "Starting");
    X_PROFILE_NO_HISTORY_BEGIN("TextureMan", core::profiler::SubSys::ENGINE3D);

    pCILoader_ = X_NEW(texture::CI::XTexLoaderCI, arena_, "CILoader");

    static_assert(texture::ImgFileFormat::ENUM_COUNT == 7, "Added additional img src fmts? this code needs updating.");

    textureLoaders_.append(pCILoader_);

    pAssetLoader_ = gEnv->pCore->GetAssetLoader();
    pAssetLoader_->registerAssetType(assetDb::AssetType::IMG, this, texture::CI_FILE_EXTENSION);

    // pointless?
    if (vars_.allowRawImgLoading()) {
        if (vars_.allowFmtDDS()) {
            textureLoaders_.append(X_NEW(texture::DDS::XTexLoaderDDS, arena_, "DDSLoader"));
        }
        if (vars_.allowFmtJPG()) {
            textureLoaders_.append(X_NEW(texture::JPG::XTexLoaderJPG, arena_, "JPGLoader"));
        }
        if (vars_.allowFmtPNG()) {
            textureLoaders_.append(X_NEW(texture::PNG::XTexLoaderPNG, arena_, "PNGLoader"));
        }
        if (vars_.allowFmtPSD()) {
            textureLoaders_.append(X_NEW(texture::PSD::XTexLoaderPSD, arena_, "PSDLoader"));
        }
        if (vars_.allowFmtTGA()) {
            textureLoaders_.append(X_NEW(texture::TGA::XTexLoaderTGA, arena_, "TGALoader"));
        }
    }

    if (!loadDefaultTextures()) {
        return false;
    }

    return true;
}

void TextureManager::shutDown(void)
{
    X_LOG0("TextureManager", "Shutting Down");

    for (auto* pTexLoader : textureLoaders_) {
        X_DELETE(pTexLoader, arena_);
    }
    textureLoaders_.clear();

    releaseDefaultTextures();
    releaseDanglingTextures();
}

bool TextureManager::asyncInitFinalize(void)
{
    // we need to know that the default textures have finished loading and are ready.
    for (auto* pTex : defaultLookup_)
    {
        if (!waitForLoad(pTex))
        {
            X_ERROR("Texture", "failed to load default texture: %s", pTex->getName().c_str());
            return false;
        }
    }

    // not needed anymore?
    if (pTexDefault_->loadFailed()) {
        X_ERROR("Texture", "failed to load default texture: %s", texture::TEX_DEFAULT_DIFFUSE);
        return false;
    }
    if (pTexDefaultBump_->loadFailed()) {
        X_ERROR("Texture", "failed to load default bump texture: %s", texture::TEX_DEFAULT_BUMP);
        return false;
    }

    return std::all_of(defaultLookup_.begin(), defaultLookup_.end(), [](Texture* pTex) { return pTex->isLoaded(); });
}

void TextureManager::scheduleStreaming(void)
{
    // we work out what to start streaming.
    if (streamQueue_.isEmpty()) {
        return;
    }
}

Texture* TextureManager::findTexture(core::string_view name) const
{
    core::ScopedLock<TextureContainer::ThreadPolicy> lock(textures_.getThreadPolicy());

    auto* pTex = textures_.findAsset(name);
    if (pTex) {
        return pTex;
    }

    X_WARNING("Texture", "Failed to find Texture: \"%*.s\"", name.length(), name.data());
    return nullptr;
}

Texture* TextureManager::loadTexture(core::string_view name, texture::TextureFlags flags)
{

    auto& threadPolicy = textures_.getThreadPolicy();
    threadPolicy.Enter();

    TexRes* pTexRes = textures_.findAsset(name);

    if (pTexRes) {
        threadPolicy.Leave();
        pTexRes->addReference();
    }
    else {
        // TODO: remove.
        core::string nameStr(name.data(), name.length());

        auto* pDevicTex = gEnv->pRender->getDeviceTexture(currentDeviceTexId_++, nameStr.c_str());
        if (!pDevicTex) {
            return nullptr;
        }

        pTexRes = textures_.createAsset(name, name, flags, pDevicTex);
        threadPolicy.Leave();

        addLoadRequest(pTexRes);
    }

    return pTexRes;
}

Texture* TextureManager::getDefault(render::TextureSlot::Enum slot) const
{
    return X_ASSERT_NOT_NULL(defaultLookup_[slot]);
}

void TextureManager::releaseTexture(Texture* pTex)
{
    TexRes* pTexRes = static_cast<TexRes*>(X_ASSERT_NOT_NULL(pTex));

    if (pTexRes->removeReference() == 0) {
        releaseResources(pTexRes);

        textures_.releaseAsset(pTexRes);
    }
}

bool TextureManager::waitForLoad(core::AssetBase* pTexture)
{
    X_ASSERT(pTexture->getType() == assetDb::AssetType::IMG, "Invalid asset passed")();

    if (pTexture->isLoaded()) {
        return true;
    }

    return waitForLoad(static_cast<Texture*>(pTexture));
}

bool TextureManager::waitForLoad(Texture* pTexture)
{
    if (pTexture->getStatus() == core::LoadStatus::Complete) {
        return true;
    }

    return pAssetLoader_->waitForLoad(pTexture);
}

bool TextureManager::loadDefaultTextures(void)
{
    using namespace texture;

    TextureFlags default_flags = TextureFlags::DONT_RESIZE | TextureFlags::DONT_STREAM;

    pTexDefault_ = loadTexture(core::string_view(TEX_DEFAULT_DIFFUSE), default_flags);
    pTexDefaultBump_ = loadTexture(core::string_view(TEX_DEFAULT_BUMP), default_flags);

    defaultLookup_.fill(pTexDefault_);
    defaultLookup_[render::TextureSlot::NORMAL] = pTexDefaultBump_;

    return true;
}

void TextureManager::releaseDefaultTextures(void)
{
    if (pTexDefault_) {
        releaseTexture(pTexDefault_);
    }
    if (pTexDefaultBump_) {
        releaseTexture(pTexDefaultBump_);
    }

    pTexDefault_ = nullptr;
    pTexDefaultBump_ = nullptr;
}

void TextureManager::addLoadRequest(TextureResource* pTexture)
{
    // TODO: use the streamQueue_, and dispatch load requests based on priority.

    pAssetLoader_->addLoadRequest(pTexture);
}

void TextureManager::onLoadRequestFail(core::AssetBase* pAsset)
{
    auto* pTexture = static_cast<Texture*>(pAsset);

    pTexture->setStatus(core::LoadStatus::Error);
}

bool TextureManager::processData(core::AssetBase* pAsset, core::UniquePointer<char[]> data, uint32_t dataSize)
{
    auto* pTexture = static_cast<Texture*>(pAsset);

#if 1
    // support the textures been compressed, stil not fully happy on this logic.
    // ideally want something more generic for compressed asset loading.
    {
        const uint8_t* pBegin = reinterpret_cast<const uint8_t*>(data.ptr());
        const uint8_t* pEnd = pBegin + dataSize;

        auto* pCompHdr = core::Compression::ICompressor::getBufferHdr(core::make_span(pBegin, dataSize));

        if (pCompHdr) {

            auto inflatedBuf = core::makeUnique<char[]>(&blockArena_, pCompHdr->inflatedSize);

            core::Compression::CompressorAlloc comp(pCompHdr->algo);

            uint8_t* pInfBegin = reinterpret_cast<uint8_t*>(inflatedBuf.ptr());
            uint8_t* pInfEnd = pInfBegin + pCompHdr->inflatedSize;

            if (!comp->inflate(arena_, pBegin, pEnd, pInfBegin, pInfEnd)) {
                X_LOG0("Texture", "failed to inflate: \"%S\"", pTexture->getName().c_str());
                pTexture->setStatus(core::LoadStatus::Error);
                return false;
            }

            dataSize = pCompHdr->inflatedSize;
            data.swap(inflatedBuf);
        }
    }
#endif

    core::XFileFixedBuf file(data.ptr(), data.ptr() + dataSize);
    texture::XTextureFile imgFile(&blockArena_);

    if (!pCILoader_->loadTexture(&file, imgFile)) {
        pTexture->setStatus(core::LoadStatus::Error);
        return false;
    }

    pTexture->setProperties(imgFile);

    // we need to upload the texture data.
    gEnv->pRender->initDeviceTexture(pTexture->getDeviceTexture(), imgFile);

    return true;
}

void TextureManager::releaseDanglingTextures(void)
{
    {
        core::ScopedLock<TextureContainer::ThreadPolicy> lock(textures_.getThreadPolicy());

        auto it = textures_.begin();
        for (; it != textures_.end(); ++it) {
            auto* pTexRes = it->second;
            releaseResources(pTexRes);
            X_WARNING("Texture", "\"%s\" was not deleted. refs: %" PRIi32, pTexRes->getName().c_str(), pTexRes->getRefCount());
        }
    }

    textures_.free();
}

void TextureManager::releaseResources(Texture* pTex)
{
    // when we release the material we need to clean up somethings.
    X_UNUSED(pTex);
}

bool TextureManager::onFileChanged(const core::AssetName& assetName, const core::string& name)
{
    X_UNUSED(assetName);

    core::ScopedLock<TextureContainer::ThreadPolicy> lock(textures_.getThreadPolicy());

    auto* pTextureRes = textures_.findAsset(name);
    if (!pTextureRes) {
        X_LOG1("Texture", "Not reloading \"%s\" it's not currently used", name.c_str());
        return false;
    }

    X_LOG0("Texture", "Reloading: %s", name.c_str());

    pAssetLoader_->reload(pTextureRes, core::ReloadFlag::Beginframe);
    return true;
}
// -----------------------------------

void TextureManager::listTextures(const char* pSearchPattern)
{
    core::ScopedLock<TextureContainer::ThreadPolicy> lock(textures_.getThreadPolicy());

    core::Array<TextureContainer::Resource*> sorted_texs(arena_);
    sorted_texs.reserve(textures_.size());

    for (const auto& mat : textures_) {
        if (!pSearchPattern || core::strUtil::WildCompare(pSearchPattern, mat.second->getName())) {
            sorted_texs.push_back(mat.second);
        }
    }

    std::sort(sorted_texs.begin(), sorted_texs.end(), [](TextureContainer::Resource* a, TextureContainer::Resource* b) {
        const auto& nameA = a->getName();
        const auto& nameB = b->getName();
        return nameA.compareInt(nameB) < 0;
    });

    X_LOG0("Texture", "------------- ^8Textures(%" PRIuS ")^7 ------------", sorted_texs.size());

    for (const auto* pTex : sorted_texs) {
        X_LOG0("Texture", "^2%-32s^7 Dim: ^2%" PRIi32 "x%" PRIi32 " ^7Mips: ^2%" PRIi32 "^7 Fmt: ^2%s ^7Refs: ^2%" PRIi32,
            pTex->getName().c_str(), pTex->getWidth(), pTex->getHeight(), pTex->getNumMips(),
            texture::Texturefmt::ToString(pTex->getFormat()), pTex->getRefCount());
    }

    X_LOG0("Texture", "------------ ^8Textures End^7 ------------");
}

// -----------------------------------

void TextureManager::Cmd_ListTextures(core::IConsoleCmdArgs* pCmd)
{
    const char* pSearchPattern = nullptr;

    if (pCmd->GetArgCount() >= 2) {
        pSearchPattern = pCmd->GetArg(1);
    }

    listTextures(pSearchPattern);
}

X_NAMESPACE_END
