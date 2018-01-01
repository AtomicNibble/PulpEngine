#pragma once

#ifndef _X_FONT_GLYPH_BITMAP_H_
#define _X_FONT_GLYPH_BITMAP_H_

X_NAMESPACE_BEGIN(font)

class XGlyphBitmap
{
public:
	typedef core::Array<uint8_t> DataVec;

public:
	FONTLIB_EXPORT XGlyphBitmap(core::MemoryArenaBase* arena);
	FONTLIB_EXPORT XGlyphBitmap(core::MemoryArenaBase* arena, int32_t width, int32_t height);
	FONTLIB_EXPORT ~XGlyphBitmap();

	FONTLIB_EXPORT void Create(int32_t width, int32_t height);
	FONTLIB_EXPORT void Release(void);

	X_INLINE DataVec& GetBuffer(void);
	X_INLINE const DataVec& GetBuffer(void) const;
	X_INLINE int32_t GetWidth(void) const;
	X_INLINE int32_t GetHeight(void) const;

	FONTLIB_EXPORT void Blur(int32_t iterations);
	FONTLIB_EXPORT bool Scale(float scaleX, float scaleY);
	// clears the buffer, no memory free
	FONTLIB_EXPORT void Clear(void);

	FONTLIB_EXPORT bool BlitScaledTo8(DataVec& destBuffer, int32_t srcX, int32_t srcY, int32_t srcWidth,
		int32_t srcHeight, int32_t destX, int32_t destY, int32_t destWidth, 
		int32_t destHeight, int32_t sestBufferWidth) const;

	FONTLIB_EXPORT bool BlitScaledTo32(DataVec& destBuffer, int32_t srcX, int32_t srcY, int32_t srcWidth,
		int32_t srcHeight, int32_t destX, int32_t destY, int32_t destWidth, 
		int32_t destHeight, int32_t sestBufferWidth) const;

	FONTLIB_EXPORT bool BlitTo8(uint8_t* pBuffer, int32_t srcX, int32_t srcY, int32_t srcWidth,
		int32_t srcHeight, int32_t destX, int32_t destY, int32_t destWidth) const;

	FONTLIB_EXPORT bool BlitTo24(uint8_t* pBuffer, int32_t srcX, int32_t srcY, int32_t srcWidth,
		int32_t srcHeight, int32_t destX, int32_t destY, int32_t destWidth) const;

	FONTLIB_EXPORT bool BlitTo32(uint32_t* pBuffer, int32_t srcX, int32_t srcY, int32_t srcWidth,
		int32_t srcHeight, int32_t destX, int32_t destY, int32_t destWidth) const;

private:
	DataVec		buffer_;
	int32_t		width_;
	int32_t		height_;
};

X_NAMESPACE_END

#include "XGlyphBitmap.inl"

#endif // !_X_FONT_GLYPH_BITMAP_H_
