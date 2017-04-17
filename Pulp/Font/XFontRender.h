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
	// Corresponds to the Unicode character set. This value covers all versions of the Unicode repertoire,
	// including ASCII and Latin-1. Most fonts include a Unicode charmap, but not all of them.
	static const FT_Encoding_ ENCODING_UNICODE = FT_ENCODING_UNICODE;
	// Corresponds to the Microsoft Symbol encoding, used to encode mathematical symbols in the 32..255 character code range.
	// For more information, see `http://www.ceviz.net/symbol.htm'.
	static const FT_Encoding_ ENCODING_SYMBOL = FT_ENCODING_MS_SYMBOL;

	typedef core::Array<uint8_t> BufferArr;

public:
	XFontRender();
	~XFontRender();

	bool SetRawFontBuffer(BufferArr& rawFontBuf);
	bool Release(void); 

	bool GetGlyph(XGlyphBitmap* pGlyphBitmap, uint8* pGlyphWidth, uint8* pGlyphHeight, 
		int8_t& iCharOffsetX, int8_t& iCharOffsetY, int32_t iX, int32_t iY, int32_t charCode);

	// scale the glyphs.
	X_INLINE void SetSizeRatio(float fSizeRatio);
	X_INLINE float GetSizeRatio(void) const;
	
	bool SetEncoding(FT_Encoding pEncoding);
	X_INLINE FT_Encoding GetEncoding(void) const;

	void SetGlyphBitmapSize(int32_t width, int32_t height);
	void GetGlyphBitmapSize(int32_t* pWidth, int32_t* pHeight) const;


private:
	static const char* errToStr(FT_Error err);

	X_NO_COPY(XFontRender);
	X_NO_ASSIGN(XFontRender);

private:
	BufferArr		fileData_; // must stay valid for lifetime of FT_Face

	FT_Library		pLibrary_;
	FT_Face			pFace_;
	FT_GlyphSlot	pGlyph_;
	FT_Encoding		pEncoding_;

	float			fSizeRatio_;

	int32_t			glyphBitmapWidth_;
	int32_t			glyphBitmapHeight_;
};


X_NAMESPACE_END

#include "XFontRender.inl"

#endif // !_X_FONT_RENDER_H_
