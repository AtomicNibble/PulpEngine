#pragma once

#ifndef _X_FONT_GLYPH_BITMAP_H_
#define _X_FONT_GLYPH_BITMAP_H_

X_NAMESPACE_BEGIN(font)

class XGlyphBitmap
{
public:
	typedef core::Array<uint8_t> DataVec;

public:
	XGlyphBitmap(core::MemoryArenaBase* arena);
	XGlyphBitmap(core::MemoryArenaBase* arena, int32_t width, int32_t height);
	~XGlyphBitmap();

	bool Create(int32_t width, int32_t height);
	void Release(void);

	X_INLINE DataVec& GetBuffer(void);
	X_INLINE const DataVec& GetBuffer(void) const;
	X_INLINE int32_t GetWidth(void) const;
	X_INLINE int32_t GetHeight(void) const;

	void Blur(int32_t iterations);
	bool Scale(float scaleX, float scaleY);
	// clears the buffer, no memory free
	void Clear(void);

	bool BlitScaledTo8(DataVec& destBuffer, int32_t srcX, int32_t srcY, int32_t srcWidth,
		int32_t srcHeight, int32_t destX, int32_t destY, int32_t destWidth, 
		int32_t destHeight, int32_t sestBufferWidth) const;

	bool BlitScaledTo32(DataVec& destBuffer, int32_t srcX, int32_t srcY, int32_t srcWidth,
		int32_t srcHeight, int32_t destX, int32_t destY, int32_t destWidth, 
		int32_t destHeight, int32_t sestBufferWidth) const;

	bool BlitTo8(uint8_t* pBuffer, int32_t srcX, int32_t srcY, int32_t srcWidth,
		int32_t srcHeight, int32_t destX, int32_t destY, int32_t destWidth) const;

	bool BlitTo32(uint32_t* pBuffer, int32_t srcX, int32_t srcY, int32_t srcWidth,
		int32_t srcHeight, int32_t destX, int32_t destY, int32_t destWidth) const;


private:
	DataVec		buffer_;
	int32_t		width_;
	int32_t		height_;
};

X_NAMESPACE_END

#include "XGlyphBitmap.inl"

#endif // !_X_FONT_GLYPH_BITMAP_H_
