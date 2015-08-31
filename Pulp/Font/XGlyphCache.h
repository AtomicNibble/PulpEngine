#pragma once

#ifndef _X_FONT_GLYPH_CACHE_H_
#define _X_FONT_GLYPH_CACHE_H_

#include <vector>
#include <unordered_map>

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

	uint8			iCharWidth;					// size in pixel
	uint8			iCharHeight;				// size in pixel
	char			iCharOffsetX;
	char			iCharOffsetY;

	XGlyphBitmap	pGlyphBitmap;

	void			Reset()
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


typedef std::unordered_map<uint16, XCacheSlot *>	XCacheTable;
typedef std::vector<XCacheSlot *>					XCacheSlotList;
typedef std::vector<XCacheSlot *>::iterator			XCacheSlotListItor;


#ifdef WIN64
#undef GetCharWidth
#undef GetCharHeight
#endif

class XGlyphCache
{
public:
	XGlyphCache();
	~XGlyphCache();

	bool Create(int iCacheSize, int iGlyphBitmapWidth, int iGlyphBitmapHeight, int iSmoothMethod, int iSmoothAmount, float fSizeRatio = 0.8f);
	void Release();

	bool LoadFontFromMemory(unsigned char *pFileBuffer, size_t iDataSize);
	void ReleaseFont();

	void GetGlyphBitmapSize(int *pWidth, int *pHeight) const;

	bool PreCacheGlyph(wchar_t cChar);
	bool UnCacheGlyph(wchar_t cChar);
	bool GlyphCached(wchar_t cChar);

	XCacheSlot* GetLRUSlot();
	XCacheSlot* GetMRUSlot();

	bool GetGlyph(XGlyphBitmap **pGlyph, int *piWidth, int *piHeight,
				char &iCharOffsetX, char &iCharOffsetY, wchar_t cChar);


	X_INLINE int SetEncoding(FT_Encoding pEncoding) { return FontRenderer_.SetEncoding(pEncoding); };
	X_INLINE FT_Encoding GetEncoding() const { return FontRenderer_.GetEncoding(); };

private:
	bool			CreateSlotList(int listSize);
	void			ReleaseSlotList();

private:
	int				iGlyphBitmapWidth_;
	int				iGlyphBitmapHeight_;
	float			fSizeRatio_;

	XCacheSlotList	SlotList_;
	XCacheTable		CacheTable_;

	XGlyphBitmap	*pScaleBitmap_;
	XFontRender		FontRenderer_;

	uint32_t		uUsage_;

	int				iSmoothMethod_;
	int				iSmoothAmount_;
};

X_NAMESPACE_END

#endif // !_X_FONT_GLYPH_CACHE_H_
