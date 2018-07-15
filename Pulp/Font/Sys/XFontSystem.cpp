#include "stdafx.h"

#include "XFontSystem.h"
#include "Font.h"
#include "FontRender\XFontTexture.h"

#include <IConsole.h>

#include <Assets\AssetLoader.h>

X_NAMESPACE_BEGIN(font)

XFontSystem::XFontSystem(ICore* pCore) :
    pCore_(pCore),
    pDefaultFont_(nullptr),
    fonts_(g_fontArena, sizeof(FontResource), X_ALIGN_OF(FontResource), "FontPool")
{
    X_ASSERT_NOT_NULL(pCore);
    X_ASSERT_NOT_NULL(g_fontArena);
}

XFontSystem::~XFontSystem()
{
}

void XFontSystem::release(void)
{
    X_DELETE(this, g_fontArena);
}

void XFontSystem::registerVars(void)
{
    vars_.registerVars();
}

void XFontSystem::registerCmds(void)
{
    // add font commands
    ADD_COMMAND_MEMBER("listFonts", this, XFontSystem, &XFontSystem::Cmd_ListFonts, core::VarFlag::SYSTEM,
        "Lists all the loaded fonts");

    ADD_COMMAND_MEMBER("fontDumpForMame", this, XFontSystem, &XFontSystem::Cmd_DumpForName, core::VarFlag::SYSTEM,
        "Dumps the font texture for a given font name");
}

bool XFontSystem::init(void)
{
    X_LOG0("FontSys", "Starting");

    pAssetLoader_ = gEnv->pCore->GetAssetLoader();
    pAssetLoader_->registerAssetType(assetDb::AssetType::FONT, this, FONT_BAKED_FILE_EXTENSION);

    gEnv->pHotReload->addfileType(this, FONT_DESC_FILE_EXTENSION);
    gEnv->pHotReload->addfileType(this, FONT_BAKED_FILE_EXTENSION);
    gEnv->pHotReload->addfileType(this, "ttf");

    // load a default font.
    pDefaultFont_ = static_cast<XFont*>(loadFont("default"));
    if (!pDefaultFont_) {
        X_ERROR("FontSys", "Failed to create default font");
        return false;
    }

    return true;
}

void XFontSystem::shutDown(void)
{
    X_LOG0("FontSys", "Shutting Down");

    freeDangling();

    gEnv->pHotReload->addfileType(nullptr, FONT_DESC_FILE_EXTENSION);
    gEnv->pHotReload->addfileType(nullptr, FONT_BAKED_FILE_EXTENSION);
    gEnv->pHotReload->addfileType(nullptr, "ttf");
}

bool XFontSystem::asyncInitFinalize(void)
{
    // potentiall we will hold this lock for a while.
    // but we should not really see any contention.
    core::ScopedLock<FontContainer::ThreadPolicy> lock(fonts_.getThreadPolicy());

    bool result = true;

    for (const auto& fontIt : fonts_) {
        auto* pFont = fontIt.second;

        // we call all even if one fails.
        result &= waitForLoad(pFont);
    }

    return result;
}

void XFontSystem::appendDirtyBuffers(render::CommandBucket<uint32_t>& bucket) const
{
    core::ScopedLock<FontContainer::ThreadPolicy> lock(fonts_.getThreadPolicy());

    for (const auto& fontIt : fonts_) {
        auto* pFont = fontIt.second;

        if (pFont->isDirty()) {
            pFont->appendDirtyBuffers(bucket);
        }
    }
}

IFont* XFontSystem::loadFont(const char* pFontName)
{
    X_ASSERT_NOT_NULL(pFontName);
    X_ASSERT(core::strUtil::FileExtension(pFontName) == nullptr, "Extension not allowed")(pFontName);

    core::string name(pFontName);
    core::ScopedLock<FontContainer::ThreadPolicy> lock(fonts_.getThreadPolicy());

    auto* pFontRes = fonts_.findAsset(name);
    if (pFontRes) {
        // inc ref count.
        pFontRes->addReference();
        return pFontRes;
    }

    pFontRes = fonts_.createAsset(name, *this, name);

    addLoadRequest(pFontRes);

    return pFontRes;
}

IFont* XFontSystem::findFont(const char* pFontName) const
{
    core::string name(pFontName);
    core::ScopedLock<FontContainer::ThreadPolicy> lock(fonts_.getThreadPolicy());

    auto* pFontRes = fonts_.findAsset(name);
    if (pFontRes) {
        return pFontRes;
    }

    X_WARNING("FontSys", "Failed to find model: \"%s\"", pFontName);
    return nullptr;
}

IFont* XFontSystem::getDefault(void) const
{
    return X_ASSERT_NOT_NULL(pDefaultFont_);
}

void XFontSystem::releaseFont(IFont* pFont)
{
    FontResource* pModelRes = static_cast<FontResource*>(pFont);
    if (pModelRes->removeReference() == 0) {
        releaseResources(pModelRes);

        fonts_.releaseAsset(pModelRes);
    }
}

bool XFontSystem::waitForLoad(IFont* pFont)
{
    FontResource* pModelRes = static_cast<FontResource*>(pFont);

    if (pModelRes->getStatus() == core::LoadStatus::Complete) {
        return true;
    }

    return pAssetLoader_->waitForLoad(pModelRes);
}

void XFontSystem::freeDangling(void)
{
    {
        core::ScopedLock<FontContainer::ThreadPolicy> lock(fonts_.getThreadPolicy());

        // any left?
        for (const auto& m : fonts_) {
            auto* pModelRes = m.second;
            const auto& name = pModelRes->getName();
            X_WARNING("XModel", "\"%s\" was not deleted. refs: %" PRIi32, name.c_str(), pModelRes->getRefCount());

            releaseResources(pModelRes);
        }
    }

    fonts_.free();
}

void XFontSystem::releaseResources(XFont* pFont)
{
    X_UNUSED(pFont);
}


void XFontSystem::addLoadRequest(FontResource* pFont)
{
    pAssetLoader_->addLoadRequest(pFont);
}

void XFontSystem::onLoadRequestFail(core::AssetBase* pAsset)
{
    X_UNUSED(pAsset);

}

bool XFontSystem::processData(core::AssetBase* pAsset, core::UniquePointer<char[]> data, uint32_t dataSize)
{
    auto* pFont = static_cast<XFont*>(pAsset);

    return pFont->processData(std::move(data), dataSize);
}

void XFontSystem::listFonts(const char* pSearchPatten) const
{
    core::ScopedLock<FontContainer::ThreadPolicy> lock(fonts_.getThreadPolicy());

    core::Array<FontResource*> sorted(g_fontArena);
    sorted.setGranularity(fonts_.size());

    for (const auto& font : fonts_) {
        auto* pFontRes = font.second;

        if (!pSearchPatten || core::strUtil::WildCompare(pSearchPatten, pFontRes->getName())) {
            sorted.push_back(pFontRes);
        }
    }

    std::sort(sorted.begin(), sorted.end(), [](FontResource* a, FontResource* b) {
        const auto& nameA = a->getName();
        const auto& nameB = b->getName();
        return nameA.compareInt(nameB) < 0;
    });

    FontFlags::Description FlagDesc;

    X_LOG0("FontSys", "---------------- ^8Fonts^7 ---------------");

    for (const auto* pFont : sorted) {
        XFontTexture* pTex = pFont->getFontTexture();
        if (pTex) {
            X_LOG0("Fonts", "Name: ^2\"%s\"^7 Flags: [^1%s^7] Size: ^2(%" PRIi32 ",%" PRIi32 ")^7 Usage: ^2%" PRIi32 " ^7CacheMiss: ^2%" PRIi32,
                pFont->getName().c_str(), pFont->getFlags().ToString(FlagDesc),
                pTex->GetWidth(), pTex->GetHeight(), pTex->GetSlotUsage(), pTex->GetCacheMisses());
        }
        else {
            X_LOG0("FontSys", "Name: ^2\"%s\"^7 Flags: [^1%s^7]", pFont->getName().c_str(), pFont->getFlags().ToString(FlagDesc));
        }
    }

    X_LOG0("FontSys", "------------- ^8Fonts End^7 --------------");
}



void XFontSystem::Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name)
{
    X_UNUSED(jobSys);
#if 0
    Path<char> path(name);
    if (strUtil::IsEqual(".font", path.extension()))
    {
        path.removeExtension();

        XFFont* pFont = static_cast<XFFont*>(GetFont(path.fileName()));
        if (pFont)
        {
            pFont->Reload();
        }
        return true;
    }

    return false;
#else
    X_UNUSED(name);
#endif
}


void XFontSystem::Cmd_ListFonts(core::IConsoleCmdArgs* pCmd)
{
    const char* pSearchPatten = nullptr;

    if (pCmd->GetArgCount() >= 2) {
        pSearchPatten = pCmd->GetArg(1);
    }

    listFonts(pSearchPatten);
}

void XFontSystem::Cmd_DumpForName(core::IConsoleCmdArgs* pCmd)
{
    size_t num = pCmd->GetArgCount();

    if (num < 2) {
        X_WARNING("FontSys", "fonts_dump_for_name <name>");
        return;
    }

    const char* pName = pCmd->GetArg(1);
    XFont* pFont = static_cast<XFont*>(findFont(pName));
    if (pFont) {
        if (pFont->getFontTexture()->WriteToFile(pName)) {
            X_LOG0("FontSys", "^8font texture successfully dumped!");
        }
    }
    else {
        X_ERROR("FontSys", "failed to dump font, no font exsists for name: %s", pName);
    }
}

X_NAMESPACE_END
