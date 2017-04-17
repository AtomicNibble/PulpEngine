#pragma once

#ifndef _X_FONT_RENDER_H_
#define _X_FONT_RENDER_H_

#define FT_EXPORT(x) x
// #define FT_EXPORT_DEF(x) x
// #define FT_EXPORT_VAR( x ) x
#include <ft2build.h>
#include FT_FREETYPE_H

X_NAMESPACE_BEGIN(font)

#define X_FONT_DEBUG_RENDER 0


class XGlyphBitmap;

// uses free type to render glyphs for a font.
class XFontRender
{
	typedef core::Array<uint8_t> BufferArr;

	X_NO_COPY(XFontRender);
	X_NO_ASSIGN(XFontRender);

public:
	XFontRender();
	~XFontRender();

	bool SetRawFontBuffer(BufferArr& rawFontBuf, FontEncoding::Enum encoding);
	bool Release(void); 

	bool GetGlyph(XGlyphBitmap* pGlyphBitmap, uint8* pGlyphWidth, uint8* pGlyphHeight, 
		int8_t& iCharOffsetX, int8_t& iCharOffsetY, int32_t iX, int32_t iY, int32_t charCode);

	// scale the glyphs.
	X_INLINE void SetSizeRatio(float fSizeRatio);
	X_INLINE float GetSizeRatio(void) const;
	
	bool SetEncoding(FontEncoding::Enum encoding);
	X_INLINE FontEncoding::Enum GetEncoding(void) const;

	void SetGlyphBitmapSize(int32_t width, int32_t height);
	void GetGlyphBitmapSize(int32_t* pWidth, int32_t* pHeight) const;


private:
	static const char* errToStr(FT_Error err);


private:
	BufferArr		fileData_; // must stay valid for lifetime of FT_Face

	FT_Library		pLibrary_;
	FT_Face			pFace_;
	FT_GlyphSlot	pGlyph_;
	FontEncoding::Enum encoding_;

	float			fSizeRatio_;

	int32_t			glyphBitmapWidth_;
	int32_t			glyphBitmapHeight_;
};


X_NAMESPACE_END

#include "XFontRender.inl"

#endif // !_X_FONT_RENDER_H_
