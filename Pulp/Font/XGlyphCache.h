#pragma once

#ifndef _X_FONT_GLYPH_CACHE_H_
#define _X_FONT_GLYPH_CACHE_H_

#include <Util\UniquePointer.h>
#include "XGlyphBitmap.h"
#include "XFontRender.h"

X_NAMESPACE_BEGIN(font)

/*
This is a wrap around the fontrender providing a cache.
in creates a cache of bitmaps created by the FreeType render code.
giving much faster text rendering at the cost of some memory.
*/

struct XCacheSlot
{

	X_INLINE void reset(void)
	{
		usage = 0;
		currentChar = static_cast<wchar_t>(~0);

		charWidth = 0;
		charHeight = 0;
		charOffsetX = 0;
		charOffsetY = 0;

		glyphBitmap.Clear();
	}

public:
	uint32			usage;
	int32_t			cacheSlot;
	wchar_t			currentChar;

	uint8_t			charWidth;					// size in pixel
	uint8_t			charHeight;				// size in pixel
	int8_t			charOffsetX;
	int8_t			charOffsetY;

	XGlyphBitmap	glyphBitmap;
};


struct FontSmooth
{
	enum Enum
	{
		NONE,
		BLUR,			// Smooth by blurring it.
		SUPERSAMPLE		// Smooth by rendering the characters into a bigger texture, and then resize it to the normal size using bilinear filtering.

	};
};

struct FontSmoothAmount
{
	enum Enum
	{
		NONE,
		X2,
		X4
	};
};


#ifdef WIN64
#undef GetCharWidth
#undef GetCharHeight
#endif

class XGlyphCache
{
	typedef core::HashMap<uint16, XCacheSlot*>			XCacheTable;
	typedef core::Array<XCacheSlot>						XCacheSlotList;
	typedef XCacheSlotList::Iterator					XCacheSlotListItor;
	typedef core::Array<uint8_t>						BufferArr;


public:
	XGlyphCache(core::MemoryArenaBase* arena);
	~XGlyphCache();

	bool Create(int32_t cacheSize, int32_t glyphBitmapWidth, int32_t glyphBitmapHeight,
		FontSmooth::Enum smoothMethod, FontSmoothAmount::Enum smoothAmount, float sizeRatio = 0.8f);
	void Release(void);

	bool LoadFontFromMemory(BufferArr& buf);
	void ReleaseFont(void);

	void GetGlyphBitmapSize(int32_t* pWidth, int32_t* pHeight) const;

	bool PreCacheGlyph(wchar_t cChar);
	bool UnCacheGlyph(wchar_t cChar);
	bool GlyphCached(wchar_t cChar);

	XCacheSlot* GetLRUSlot(void);
	XCacheSlot* GetMRUSlot(void);

	bool GetGlyph(XGlyphBitmap*& pGlyphOut, int32_t* pWidth, int32_t* pHeight,
		int8_t& charOffsetX, int8_t& charOffsetY, wchar_t cChar);


	X_INLINE bool SetEncoding(FT_Encoding pEncoding);
	X_INLINE FT_Encoding GetEncoding(void) const;

private:
	bool CreateSlotList(size_t listSize);
	void ReleaseSlotList(void);

private:
	core::MemoryArenaBase* arena_;

	int32_t			glyphBitmapWidth_;
	int32_t			glyphBitmapHeight_;
	float			fSizeRatio_;

	XCacheSlotList	slotList_;
	XCacheTable		cacheTable_;

	core::UniquePointer<XGlyphBitmap> scaleBitmap_;
	XFontRender		fontRenderer_;

	uint32_t		usage_;

	int32_t			smoothMethod_;
	int32_t			smoothAmount_;
};

X_NAMESPACE_END

#include "XGlyphCache.inl"

#endif // !_X_FONT_GLYPH_CACHE_H_
