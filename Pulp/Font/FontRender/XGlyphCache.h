#pragma once

#ifndef _X_FONT_GLYPH_CACHE_H_
#define _X_FONT_GLYPH_CACHE_H_

#include <Util\UniquePointer.h>
#include <Containers\HashMap.h>

X_NAMESPACE_BEGIN(font)

class FontVars;

#ifdef WIN64
#undef GetCharWidth
#undef GetCharHeight
#endif

class XGlyphCache
{
    typedef core::HashMap<uint16, XGlyph*> XCacheTable;
    typedef core::Array<XGlyph> XGlyphArr;
    typedef XGlyphArr::Iterator XGlyphArrItor;
    typedef core::Array<uint8_t> BufferArr;

public:
    XGlyphCache(const FontVars& vars, core::MemoryArenaBase* arena);
    ~XGlyphCache();

    X_INLINE bool IsLoaded(void) const;

    X_INLINE bool SetEncoding(FontEncoding::Enum encoding);
    X_INLINE FontEncoding::Enum GetEncoding(void) const;
    X_INLINE const Metrics& GetMetrics(void) const;

    bool Create(int32_t glyphBitmapWidth, int32_t glyphBitmapHeight);

    bool SetRawFontBuffer(core::span<const uint8_t> fontSrc,
        FontEncoding::Enum encoding, float sizeRatio = 1.0f);
    bool SetBakedData(core::span<const GlyphHdr> bakedGlyphs, core::span<const char> bakedData);

    void GetGlyphBitmapSize(int32_t* pWidth, int32_t* pHeight) const;

    bool PreCacheGlyph(wchar_t cChar);
    bool UnCacheGlyph(wchar_t cChar);
    bool GlyphCached(wchar_t cChar);

    XGlyph* GetLRUSlot(void);
    XGlyph* GetMRUSlot(void);

    const XGlyph* GetGlyph(wchar_t cChar);

private:
    bool CreateSlotList(size_t listSize);
    void ReleaseSlotList(void);

private:
    const FontVars& vars_;
    XFontRender fontRenderer_;
    core::UniquePointer<XGlyphBitmap> scaleBitmap_;
    Metrics metrics_;

    int32_t glyphBitmapWidth_;
    int32_t glyphBitmapHeight_;
    int32_t scaledGlyphWidth_;
    int32_t scaledGlyphHeight_;

    XGlyphArr slotList_;
    XCacheTable cacheTable_;

    uint32_t usage_;

    FontSmooth::Enum smoothMethod_;
    FontSmoothAmount::Enum smoothAmount_;
};

X_NAMESPACE_END

#include "XGlyphCache.inl"

#endif // !_X_FONT_GLYPH_CACHE_H_
