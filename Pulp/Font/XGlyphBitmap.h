#pragma once

#ifndef _X_FONT_GLYPH_BITMAP_H_
#define _X_FONT_GLYPH_BITMAP_H_

X_NAMESPACE_BEGIN(font)

class XGlyphBitmap
{
public:
	XGlyphBitmap();
	XGlyphBitmap(int width, int height);
	~XGlyphBitmap();

	bool Create(int width, int height);
	void Release();

	X_INLINE uint8_t* GetBuffer() { return pBuffer_; };
	X_INLINE int GetWidth() const { return width_; }
	X_INLINE int GetHeight() const { return height_; }

	void Blur(int iterations);
	bool Scale(float scaleX, float scaleY);
	// clears the buffer, no memory free
	void Clear();

	bool BlitScaledTo8(uint8_t* pBuffer, int iSrcX, int iSrcY, int iSrcWidth,
		int iSrcHeight, int iDestX, int iDestY, int iDestWidth, 
		int iDestHeight, int iDestBufferWidth);

	bool BlitScaledTo32(uint8_t* pBuffer, int iSrcX, int iSrcY, int iSrcWidth,
		int iSrcHeight, int iDestX, int iDestY, int iDestWidth, 
		int iDestHeight, int iDestBufferWidth);

	bool BlitTo8(uint8_t* pBuffer, int iSrcX, int iSrcY, int iSrcWidth,
		int iSrcHeight, int iDestX, int iDestY, int iDestWidth);

	bool BlitTo32(uint32_t* pBuffer, int iSrcX, int iSrcY, int iSrcWidth,
		int iSrcHeight, int iDestX, int iDestY, int iDestWidth);


private:
	uint8_t*	pBuffer_;
	int			width_;
	int			height_;
};

X_NAMESPACE_END

#endif // !_X_FONT_GLYPH_BITMAP_H_
