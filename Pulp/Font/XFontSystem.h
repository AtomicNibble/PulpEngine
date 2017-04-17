#pragma once

#ifndef _X_XFONNT_H_
#define _X_XFONNT_H_

#include <Platform\DirectoryWatcher.h>
#include <Containers\HashMap.h>
#include <Util\ReferenceCounted.h>

#include "Vars\FontVars.h"

X_NAMESPACE_BEGIN(font)

class XFont;
class XFontTexture;
class XGlyphCache;

class XFontSystem : public IFontSys, public core::IXHotReload
{
//	typedef core::ReferenceCountedInstance<XFontTexture*> XFontTextureRef;
//	typedef core::ReferenceCountedInstance<XGlyphCache*> XGlyphCacheRef;

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

	// IXFont
	virtual void appendDirtyBuffers(render::CommandBucket<uint32_t>& bucket) const X_FINAL;

	virtual IFont* NewFont(const char* pFontName) X_FINAL;
	virtual IFont* GetFont(const char* pFontName) const X_FINAL;
	virtual void ListFonts(void) const X_FINAL;
	// ~IXFont

	// IXHotReload
	// returns true if it action was eaten.
	void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_OVERRIDE;
	// ~IXHotReload

	XFontTexture* getFontTexture(const SourceNameStr& name, bool async);
	void releaseFontTexture(XFontTexture* pFontTex);


private:
	ICore* pCore_;
	FontMap fonts_;

	FontVars vars_;

	core::CriticalSection lock_;
	FontTextureMap fontTextures_;
};

X_NAMESPACE_END

#endif // !_X_XFONNT_H_
