#pragma once

#ifndef _X_FONT_SYS_H_
#define _X_FONT_SYS_H_

#include <String\StackString.h>
#include <Containers\FixedArray.h>
#include <Containers\Array.h>

#include <Util\UniquePointer.h>

struct Vertex_P3F_T2F_C4B;

X_NAMESPACE_DECLARE(engine,
	class Material;
);

X_NAMESPACE_BEGIN(font)

class XFontTexture;
class XFontSystem;

class XFont : public IFont
{
public:
	friend class XFont;

	static const size_t MAX_TXT_SIZE = 1024;
	static const size_t MAX_FONT_PASS = 4;
	static const int32_t FONT_QUAD_BUFFER_SIZE = 6 * 64;
	static const int32_t FONT_TAB_CHAR_NUM = 4;
	static const float FONT_SPACE_SIZE;
	static const float FONT_GLYPH_PROP_SPACING;

	struct FontPass
	{
		FontPass() : col(255, 255, 255, 255) { }
		Color8u col;
		Vec2f offset;
	};

	struct FontEffect
	{
		core::StackString<64> name;
		core::FixedArray<FontPass, MAX_FONT_PASS> passes;
	};

	typedef core::Array<FontEffect> EffetsArr;

public:
	XFont(ICore* pCore, XFontSystem& fontSys, const char* pFontName);
	~XFont();


	// IFont
	void Release(void) X_FINAL;
	void Free(void) X_FINAL;
	void FreeBuffers(void) X_FINAL;
	void FreeTexture(void) X_FINAL;

	bool loadFont(void) X_FINAL;

	void DrawString(engine::IPrimativeContext* pPrimCon, const Vec3f& pos,
		const XTextDrawConect& contex, const char* pBegin, const char* pEnd) X_FINAL;
	void DrawString(engine::IPrimativeContext* pPrimCon, const Vec3f& pos,
		const XTextDrawConect& contex, const wchar_t* pBegin, const wchar_t* pEnd) X_FINAL;

	size_t GetTextLength(const char* pBegin, const char* pEnd, const bool asciiMultiLine) const X_FINAL;
	size_t GetTextLength(const wchar_t* pBegin, const wchar_t* pEnd, const bool asciiMultiLine) const X_FINAL;

	// calculate the size.
	Vec2f GetTextSize(const char* pBegin, const char* pEnd, const XTextDrawConect& contex) X_FINAL;
	Vec2f GetTextSize(const wchar_t* pBegin, const wchar_t* pEnd, const XTextDrawConect& contex) X_FINAL;

	int32_t GetEffectId(const char* pEffectName) const X_FINAL;

	// ~IFont

	void GetGradientTextureCoord(float& minU, float& minV, 
		float& maxU, float& maxV) const;

	X_INLINE const char* getName(void) const;
	X_INLINE bool isDirty(void) const;
	X_INLINE bool isTextureValid(void) const;
	X_INLINE XFontTexture* getFontTexture(void) const;

	void appendDirtyBuffers(render::CommandBucket<uint32_t>& bucket);

private:
	bool processXmlData(const char* pBegin, const char* pEnd);

private:
	Vec2f GetTextSizeWInternal(const wchar_t* pBegin, const wchar_t* pEnd, const XTextDrawConect& contex);

	bool CreateDeviceTexture(void);
	bool InitCache(void);
	void Prepare(const wchar_t* pBegin, const wchar_t* pEnd);
	void Reload(void);

	bool loadTTF(const char* pFilePath, uint32_t width, uint32_t height);

private:
	static size_t ByteToWide(const char* pBegin, const char* pEnd, wchar_t* pOut, const size_t buflen);


private:
	core::StackString<128> name_;
	core::StackString<128> sourceName_;

	ICore* pCore_;
	XFontSystem& fontSys_;
	
	core::UniquePointer<XFontTexture> fontTexture_;
	EffetsArr effects_;

	texture::ITexture* pTexture_;
	bool fontTexDirty_;

	// shader and state.
	engine::Material* pMaterial_;
};

X_INLINE const char* XFont::getName(void) const
{ 
	return name_.c_str();
}

X_INLINE bool XFont::isDirty(void) const
{
	return fontTexDirty_;
}

X_INLINE XFontTexture* XFont::getFontTexture(void) const
{ 
	return fontTexture_.ptr();
}

X_NAMESPACE_END

#endif // !_X_FONT_SYS_H_
