#pragma once

#ifndef _X_FONT_SYS_H_
#define _X_FONT_SYS_H_

#include <String\StackString.h>
#include <Containers\FixedArray.h>
#include <Containers\Array.h>

struct Vertex_P3F_T2F_C4B;

X_NAMESPACE_BEGIN(font)

class XFontTexture;
class XFontSystem;

class XFont : public IFont, public IXFont_RenderProxy
{
public:
	friend class XFont;

	static const size_t MAX_TXT_SIZE = 1024;
	static const size_t MAX_FONT_PASS = 4;
	static const int32_t FONT_QUAD_BUFFER_SIZE = 6 * 64;
	static const int32_t FONT_TAB_CHAR_NUM = 4;
	static const float FONT_SPACE_SIZE;
	static const float FONT_GLYPH_PROP_SPACING;


public:
	XFont(ICore* pCore, XFontSystem* pFontSys, const char* pFontName);
	~XFont();

	struct FontPass
	{
		FontPass() : col(255,255,255,255) { }
		Color8u col;
		Vec2f offset;
	};
	
	struct FontEffect
	{
		core::StackString<64> name;
		core::FixedArray<FontPass, MAX_FONT_PASS> passes;
	};


	// IFont
	virtual void Release(void) X_OVERRIDE;
	virtual void Free(void) X_OVERRIDE;
	virtual void FreeBuffers(void) X_OVERRIDE;
	virtual void FreeTexture(void) X_OVERRIDE;

//	virtual bool loadTTF(const char* pFilePath, uint32_t width, uint32_t height) X_OVERRIDE;
	virtual bool loadFont(void) X_OVERRIDE;

	virtual void DrawString(const Vec2f& pos, const char* pStr, const XTextDrawConect& contex) X_OVERRIDE;
	virtual void DrawString(float x, float y, const char* pStr, const XTextDrawConect& contex) X_OVERRIDE;
	virtual void DrawString(const Vec3f& pos, const char* pStr, const XTextDrawConect& contex) X_OVERRIDE;

	virtual void DrawStringW(const Vec2f& pos, const wchar_t* pStr, const XTextDrawConect& contex) X_OVERRIDE;
	virtual void DrawStringW(const Vec3f& pos, const wchar_t* pStr, const XTextDrawConect& contex) X_OVERRIDE;

	virtual size_t GetTextLength(const char* pStr, const bool asciiMultiLine) const X_OVERRIDE;
	virtual size_t GetTextLengthW(const wchar_t* pStr, const bool asciiMultiLine) const X_OVERRIDE;

	// calculate the size.
	virtual Vec2f GetTextSize(const char* pStr, const XTextDrawConect& contex) X_OVERRIDE;
	virtual Vec2f GetTextSizeW(const wchar_t* pStr, const XTextDrawConect& contex) X_OVERRIDE;

	virtual uint32_t GetEffectId(const char* pEffectName) const X_OVERRIDE;

	virtual void GetGradientTextureCoord(float& minU, float& minV, 
		float& maxU, float& maxV) const X_OVERRIDE;
	// ~IFont

	// IXFont_RenderProxy
	virtual void RenderCallback(const Vec3f& pos, const wchar_t* pStr, const XTextDrawConect& ctx);

	X_INLINE const char* getName(void) const { return name_.c_str(); }
	X_INLINE XFontTexture* getFontTexture(void) const { return pFontTexture_; }

private:
	void DrawStringWInternal(const Vec3f& pos, const wchar_t* pStr, const XTextDrawConect& contex);
	Vec2f GetTextSizeWInternal(const wchar_t* pStr, const XTextDrawConect& contex);

	bool InitTexture(void);
	bool InitCache(void);
	void Prepare(const wchar_t* pStr, bool updateTexture);
	void Reload(void);

	bool loadTTF(const char* pFilePath, uint32_t width, uint32_t height);

private:
	static void ByteToWide(const char* pStr, wchar_t* pOut, const size_t buflen);


private:
	typedef core::Array<FontEffect> Effets;

	core::StackString<128> name_;
	core::StackString<128> sourceName_;

	ICore* pCore_;
	XFontSystem* pFontSys_;
	
	XFontTexture* pFontTexture_;
	uint8_t* FontBuffer_;

	int texID_;
	bool fontTexDirty_;

	Effets effects_;

	Vertex_P3F_T2F_C4B* pVertBuffer_;
};


X_NAMESPACE_END

#endif // !_X_FONT_SYS_H_
