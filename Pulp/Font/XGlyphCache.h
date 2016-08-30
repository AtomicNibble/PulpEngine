#pragma once

#ifndef _X_FONT_GLYPH_CACHE_H_
#define _X_FONT_GLYPH_CACHE_H_


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
	uint32			uUsage;
	int				iCacheSlot;
	wchar_t			cCurrentChar;

	uint8_t			iCharWidth;					// size in pixel
	uint8_t			iCharHeight;				// size in pixel
	int8_t			iCharOffsetX;
	int8_t			iCharOffsetY;

	XGlyphBitmap	pGlyphBitmap;

	void reset(void)
	{
		uUsage = 0;
		cCurrentChar = static_cast<wchar_t>(~0);

		iCharWidth = 0;
		iCharHeight = 0;
		iCharOffsetX = 0;
		iCharOffsetY = 0;

		pGlyphBitmap.Clear();
	}
};


#ifdef WIN64
#undef GetCharWidth
#undef GetCharHeight
#endif

class XGlyphCache
{
	typedef core::HashMap<uint16, XCacheSlot *>			XCacheTable;
	typedef core::Array<XCacheSlot *>					XCacheSlotList;
	typedef core::Array<XCacheSlot *>::Iterator			XCacheSlotListItor;


public:
	XGlyphCache(core::MemoryArenaBase* arena);
	~XGlyphCache();

	bool Create(int32_t iCacheSize, int32_t iGlyphBitmapWidth, int32_t iGlyphBitmapHeight,
		int32_t iSmoothMethod, int32_t iSmoothAmount, float fSizeRatio = 0.8f);
	void Release(void);

	bool LoadFontFromMemory(uint8_t* pFileBuffer, size_t iDataSize);
	void ReleaseFont(void);

	void GetGlyphBitmapSize(int32_t* pWidth, int32_t* pHeight) const;

	bool PreCacheGlyph(wchar_t cChar);
	bool UnCacheGlyph(wchar_t cChar);
	bool GlyphCached(wchar_t cChar);

	XCacheSlot* GetLRUSlot(void);
	XCacheSlot* GetMRUSlot(void);

	bool GetGlyph(XGlyphBitmap** pGlyph, int32_t* piWidth, int32_t* piHeight,
		int8_t& iCharOffsetX, int8_t& iCharOffsetY, wchar_t cChar);


	X_INLINE int SetEncoding(FT_Encoding pEncoding);
	X_INLINE FT_Encoding GetEncoding(void) const;

private:
	bool			CreateSlotList(size_t listSize);
	void			ReleaseSlotList(void);

private:
	core::MemoryArenaBase* arena_;

	int32_t			iGlyphBitmapWidth_;
	int32_t			iGlyphBitmapHeight_;
	float			fSizeRatio_;

	XCacheSlotList	SlotList_;
	XCacheTable		CacheTable_;

	XGlyphBitmap*	pScaleBitmap_;
	XFontRender		fontRenderer_;

	uint32_t		uUsage_;

	int32_t			iSmoothMethod_;
	int32_t			iSmoothAmount_;
};

X_NAMESPACE_END

#include "XGlyphCache.inl"

#endif // !_X_FONT_GLYPH_CACHE_H_
