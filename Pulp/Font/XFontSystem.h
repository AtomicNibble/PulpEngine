#pragma once

#ifndef _X_XFONNT_H_
#define _X_XFONNT_H_

#include <Platform\DirectoryWatcher.h>
#include <Containers\HashMap.h>

X_NAMESPACE_BEGIN(font)

class XFont;

class XFontSystem : public IFontSys, public core::IXHotReload
{
	typedef core::HashMap<core::string, XFont*> FontMap;
	typedef FontMap::iterator FontMapItor;
	typedef FontMap::const_iterator FontMapConstItor;

public:
	XFontSystem(ICore* pCore);
	virtual ~XFontSystem();

	// IXFont
	virtual bool Init(void) X_OVERRIDE;
	virtual void ShutDown(void) X_OVERRIDE;
	virtual void release(void) X_OVERRIDE;

	virtual void appendDirtyBuffers(render::CommandBucket<uint32_t>& bucket) const X_OVERRIDE;

	virtual IFont* NewFont(const char* pFontName) X_OVERRIDE;
	virtual IFont* GetFont(const char* pFontName) const X_OVERRIDE;
	virtual void ListFontNames(void) const X_OVERRIDE;
	// ~IXFont

	// IXHotReload
	// returns true if it action was eaten.
	void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_OVERRIDE;
	// ~IXHotReload

private:
	ICore* pCore_;
	FontMap fonts_;
};

X_NAMESPACE_END

#endif // !_X_XFONNT_H_
