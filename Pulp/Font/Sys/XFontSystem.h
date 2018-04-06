#pragma once

#ifndef _X_XFONNT_H_
#define _X_XFONNT_H_

#include <Platform\DirectoryWatcher.h>
#include <Containers\HashMap.h>
#include <Util\ReferenceCounted.h>

#include "Vars\FontVars.h"

X_NAMESPACE_DECLARE(core, struct IConsoleCmdArgs)

X_NAMESPACE_BEGIN(font)

class XFont;
class XFontTexture;
class XGlyphCache;

class XFontSystem : public IFontSys
    , public core::IXHotReload
{
    typedef core::HashMap<FontNameStr, XFont*> FontMap;
    typedef core::HashMap<SourceNameStr, XFontTexture*> FontTextureMap;

    typedef FontMap::iterator FontMapItor;
    typedef FontMap::const_iterator FontMapConstItor;

public:
    XFontSystem(ICore* pCore);
    virtual ~XFontSystem();

    // IEngineSysBase
    virtual void registerVars(void) X_FINAL;
    virtual void registerCmds(void) X_FINAL;

    virtual bool init(void) X_FINAL;
    virtual void shutDown(void) X_FINAL;
    virtual void release(void) X_FINAL;

    virtual bool asyncInitFinalize(void) X_FINAL;

    // IXFont
    virtual void appendDirtyBuffers(render::CommandBucket<uint32_t>& bucket) const X_FINAL;

    virtual IFont* NewFont(const char* pFontName) X_FINAL;
    virtual IFont* GetFont(const char* pFontName) const X_FINAL;
    virtual IFont* GetDefault(void) const X_FINAL;
    virtual void ListFonts(void) const X_FINAL;
    // ~IXFont

    // IXHotReload
    // returns true if it action was eaten.
    void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_OVERRIDE;
    // ~IXHotReload

    XFontTexture* getFontTexture(const SourceNameStr& name, bool async);
    void releaseFontTexture(XFontTexture* pFontTex);

    X_INLINE const FontVars& getVars(void) const;

private:
    void Command_ListFonts(core::IConsoleCmdArgs* pCmd);
    void Command_DumpForName(core::IConsoleCmdArgs* pCmd);

private:
    ICore* pCore_;
    XFont* pDefaultFont_;
    FontMap fonts_;

    FontVars vars_;

    mutable core::CriticalSection lock_;
    FontTextureMap fontTextures_;
};

X_INLINE const FontVars& XFontSystem::getVars(void) const
{
    return vars_;
}

X_NAMESPACE_END

#endif // !_X_XFONNT_H_
