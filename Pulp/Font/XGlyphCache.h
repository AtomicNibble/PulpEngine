#pragma once

#ifndef _X_FONT_GLYPH_CACHE_H_
#define _X_FONT_GLYPH_CACHE_H_

#include <Util\UniquePointer.h>
#include <Util\ReferenceCounted.h>
#include <Threading\Signal.h>

#include "XGlyphBitmap.h"
#include "XFontRender.h"

#include <Containers\HashMap.h>

X_NAMESPACE_DECLARE(core,
	struct IoRequestBase;
	struct XFileAsync;
);


X_NAMESPACE_BEGIN(font)

class FontVars;

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


#ifdef WIN64
#undef GetCharWidth
#undef GetCharHeight
#endif

class XGlyphCache : public core::ReferenceCounted<>
{
	typedef core::HashMap<uint16, XCacheSlot*>			XCacheTable;
	typedef core::Array<XCacheSlot>						XCacheSlotList;
	typedef XCacheSlotList::Iterator					XCacheSlotListItor;
	typedef core::Array<uint8_t>						BufferArr;


public:
	XGlyphCache(const FontVars& vars, core::MemoryArenaBase* arena);
	~XGlyphCache();

	X_INLINE bool IsLoaded(void) const;
	bool WaitTillReady(void);

	X_INLINE bool SetEncoding(FontEncoding::Enum encoding);
	X_INLINE FontEncoding::Enum GetEncoding(void) const;

	bool LoadGlyphSource(const SourceNameStr& name, bool async);

	bool Create(int32_t glyphBitmapWidth, int32_t glyphBitmapHeight);
	void Release(void);

	void GetGlyphBitmapSize(int32_t* pWidth, int32_t* pHeight) const;

	void PreWarmCache(void);
	bool PreCacheGlyph(wchar_t cChar);
	bool UnCacheGlyph(wchar_t cChar);
	bool GlyphCached(wchar_t cChar);

	XCacheSlot* GetLRUSlot(void);
	XCacheSlot* GetMRUSlot(void);

	bool GetGlyph(XGlyphBitmap*& pGlyphOut, int32_t* pWidth, int32_t* pHeight,
		int8_t& charOffsetX, int8_t& charOffsetY, wchar_t cChar);

private:
	bool CreateSlotList(size_t listSize);
	void ReleaseSlotList(void);

private:
	void IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
		core::XFileAsync* pFile, uint32_t bytesTransferred);

	void ProcessFontFile_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);

private:
	const FontVars& vars_;
	XFontRender		fontRenderer_;
	core::UniquePointer<XGlyphBitmap> scaleBitmap_;

	int32_t			glyphBitmapWidth_;
	int32_t			glyphBitmapHeight_;
	int32_t			scaledGlyphWidth_;
	int32_t			scaledGlyphHeight_;

	XCacheSlotList	slotList_;
	XCacheTable		cacheTable_;

	uint32_t		usage_;

	FontSmooth::Enum smoothMethod_;
	FontSmoothAmount::Enum	smoothAmount_;

	core::Signal signal_;
	LoadStatus::Enum loadStatus_;
};

X_NAMESPACE_END

#include "XGlyphCache.inl"

#endif // !_X_FONT_GLYPH_CACHE_H_
