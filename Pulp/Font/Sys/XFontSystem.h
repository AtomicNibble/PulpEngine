#pragma once

#ifndef _X_XFONNT_H_
#define _X_XFONNT_H_

#include <Platform\DirectoryWatcher.h>
#include <Util\ReferenceCounted.h>
#include <Assets\AssertContainer.h>

#include "Vars\FontVars.h"

X_NAMESPACE_DECLARE(core, struct IConsoleCmdArgs)

X_NAMESPACE_BEGIN(font)

class XFont;
class XFontTexture;
class XGlyphCache;

class XFontSystem : public IFontSys
    , private core::IAssetLoadSink
{
    typedef core::AssetContainer<XFont, FONT_MAX_LOADED, core::SingleThreadPolicy> FontContainer;
    typedef FontContainer::Resource FontResource;

    typedef core::HashMap<SourceNameStr, XFontTexture*> FontTextureMap;

public:
    XFontSystem(ICore* pCore);
    virtual ~XFontSystem();

    // IEngineSysBase
    void registerVars(void) X_FINAL;
    void registerCmds(void) X_FINAL;

    bool init(void) X_FINAL;
    void shutDown(void) X_FINAL;
    void release(void) X_FINAL;

    bool asyncInitFinalize(void) X_FINAL;

    // IXFont
    void appendDirtyBuffers(render::CommandBucket<uint32_t>& bucket) const X_FINAL;

    IFont* loadFont(core::string_view name) X_FINAL;
    IFont* findFont(core::string_view name) const X_FINAL;
    IFont* getDefault(void) const X_FINAL;

    void releaseFont(IFont* pFont) X_FINAL;

    bool waitForLoad(IFont* pFont) X_FINAL;

    void listFonts(core::string_view searchPattern) const X_FINAL;
    // ~IXFont

    X_INLINE const FontVars& getVars(void) const;

private:
    void freeDangling(void);
    void releaseResources(XFont* pFont);

    void addLoadRequest(FontResource* pFont);
    void onLoadRequestFail(core::AssetBase* pAsset) X_FINAL;
    bool processData(core::AssetBase* pAsset, core::UniquePointer<char[]> data, uint32_t dataSize) X_FINAL;
    bool onFileChanged(const core::AssetName& assetName, const core::string& name) X_FINAL;

    void Cmd_ListFonts(core::IConsoleCmdArgs* pCmd);
    void Cmd_DumpForName(core::IConsoleCmdArgs* pCmd);

private:
    ICore* pCore_;
    XFont* pDefaultFont_;
    FontVars vars_;

    FontContainer fonts_;

    core::AssetLoader* pAssetLoader_;
};

X_INLINE const FontVars& XFontSystem::getVars(void) const
{
    return vars_;
}

X_NAMESPACE_END

#endif // !_X_XFONNT_H_
