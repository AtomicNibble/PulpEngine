#pragma once

#ifndef _X_FONT_NULL_H_
#define _X_FONT_NULL_H_

X_NAMESPACE_BEGIN(font)

#if defined(X_USE_NULLFONT)

class XFontNull : public IFFont
{
public:
	virtual void Release() {};
	virtual void Free() {};

	virtual bool load(const char* pFilePath, uint32_t width, uint32_t height) { return true; };

	// expose drawing shit baby.
	virtual void DrawString(const Vec2f& pos, const char* pStr, const XTextDrawConect& contex) {};
	virtual void DrawString(const Vec3f& pos, const char* pStr, const XTextDrawConect& contex) {};

	// calculate the size.
	virtual Vec2f GetTextSize(const char* pStr, const XTextDrawConect& contex) {
		return Vec2f::zero();
	}

};


class XFontSysNull : public IXFontSys
{
public:
	virtual void Init() X_OVERRIDE {}
	virtual void ShutDown() X_OVERRIDE{}
	virtual void release() X_OVERRIDE { X_DELETE(this, g_fontArena); }


	virtual IFFont* NewFont(const char* pFontName) X_OVERRIDE { return &nullFont_; };
	virtual IFFont* GetFont(const char* pFontName) const X_OVERRIDE { return &nullFont_; };

private:
	static XFontNull nullFont_;
};

#endif // X_USE_NULLFONT

X_NAMESPACE_END

#endif // !_X_FONT_NULL_H_
