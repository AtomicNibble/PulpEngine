#pragma once

#ifndef _X_XFONNT_H_
#define _X_XFONNT_H_

#include <Platform\DirectoryWatcher.h>
#include <Containers\HashMap.h>

X_NAMESPACE_BEGIN(font)

class XFFont;

class XFont : public IXFontSys, public core::IXHotReload
{
	friend class XFFont;

public:
	XFont(ICore* pCore);
	virtual ~XFont();

	// IXFont
	virtual void Init() X_OVERRIDE;
	virtual void ShutDown() X_OVERRIDE;
	virtual void release() X_OVERRIDE;

	virtual IFFont* NewFont(const char* pFontName) X_OVERRIDE;
	virtual IFFont* GetFont(const char* pFontName) const X_OVERRIDE;
	virtual void ListFontNames(void) const X_OVERRIDE;
	// ~IXFont

	// IXHotReload
	// returns true if it action was eaten.
	virtual bool OnFileChange(const char* name) X_OVERRIDE;
	// ~IXHotReload

private:
	ICore* pCore_;

	typedef core::HashMap<core::string, XFFont*> FontMap;
	typedef FontMap::iterator FontMapItor;
	typedef FontMap::const_iterator FontMapConstItor;

	FontMap fonts_;
};

X_NAMESPACE_END

#endif // !_X_XFONNT_H_
