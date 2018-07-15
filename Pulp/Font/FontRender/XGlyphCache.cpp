#include "stdafx.h"
#include "XGlyphCache.h"
#include "XFontTexture.h"

#include "Vars\FontVars.h"

X_NAMESPACE_BEGIN(font)

namespace
{
    int32_t getOffsetMulti(FontSmoothAmount::Enum smoothAmount)
    {
        int32_t offsetMult = 1;

        switch (smoothAmount) {
            case FontSmoothAmount::X2:
                offsetMult = 2;
                break;
            case FontSmoothAmount::X4:
                offsetMult = 4;
                break;
        }
        return offsetMult;
    }

} // namespace

XGlyphCache::XGlyphCache(const FontVars& vars, core::MemoryArenaBase* arena) :
    vars_(vars),
    fontRenderer_(arena),
    glyphBitmapWidth_(0),
    glyphBitmapHeight_(0),
    scaledGlyphWidth_(0),
    scaledGlyphHeight_(0),

    scaleBitmap_(arena),

    usage_(0),

    smoothMethod_(FontSmooth::NONE),
    smoothAmount_(FontSmoothAmount::NONE),

    slotList_(arena),
    cacheTable_(arena, 8)
{
    core::zero_object(metrics_);
}

XGlyphCache::~XGlyphCache()
{
}


bool XGlyphCache::Create(int32_t glyphBitmapWidth, int32_t glyphBitmapHeight)
{
    int32_t cacheSize = vars_.glyphCacheSize();

    smoothMethod_ = vars_.fontSmoothMethod();
    smoothAmount_ = vars_.fontSmoothAmount();

    glyphBitmapWidth_ = glyphBitmapWidth;
    glyphBitmapHeight_ = glyphBitmapHeight;
    scaledGlyphWidth_ = 0;
    scaledGlyphHeight_ = 0;

    if (!CreateSlotList(cacheSize)) {
        ReleaseSlotList();
        return false;
    }

    switch (smoothMethod_) {
        case FontSmooth::SUPERSAMPLE: {
            switch (smoothAmount_) {
                case FontSmoothAmount::X2:
                    scaledGlyphWidth_ = glyphBitmapWidth_ << 1;
                    scaledGlyphHeight_ = glyphBitmapHeight_ << 1;
                    break;
                case FontSmoothAmount::X4:
                    scaledGlyphWidth_ = glyphBitmapWidth_ << 2;
                    scaledGlyphHeight_ = glyphBitmapHeight_ << 2;
                    break;
            }
        } break;
        default:
            break;
    }

    // Scaled?
    if (scaledGlyphWidth_ > 0) {
        scaleBitmap_.reset(X_NEW(XGlyphBitmap, scaleBitmap_.getArena(), "BitMap")(g_fontArena));
        scaleBitmap_->Create(scaledGlyphWidth_, scaledGlyphHeight_);
    }

    return true;
}

bool XGlyphCache::SetRawFontBuffer(core::span<const uint8_t> fontSrc, FontEncoding::Enum encoding, float sizeRatio)
{
    if (!fontRenderer_.SetRawFontBuffer(fontSrc, encoding)) {
        X_ERROR("Font", "Error setting up font renderer");
        return false;
    }

    if (scaledGlyphWidth_) {
        fontRenderer_.SetGlyphBitmapSize(scaledGlyphWidth_, scaledGlyphHeight_, sizeRatio);
    }
    else {
        fontRenderer_.SetGlyphBitmapSize(glyphBitmapWidth_, glyphBitmapHeight_, sizeRatio);
    }

    metrics_ = fontRenderer_.GetMetrics();

    if (smoothMethod_ == FontSmooth::SUPERSAMPLE) {
        auto offsetMult = getOffsetMulti(smoothAmount_);

        metrics_.ascender >>= offsetMult >> 1;
        metrics_.descender >>= offsetMult >> 1;
        metrics_.maxAdvance >>= offsetMult >> 1;
    }

    return true;
}

bool XGlyphCache::SetBakedData(core::span<const GlyphHdr> bakedGlyphs, core::span<const char> bakedData)
{
    X_UNUSED(bakedGlyphs, bakedData);

    if (bakedGlyphs.empty()) {
        return true;
    }

    auto usage = ++usage_;
    auto bitMapSize = glyphBitmapWidth_ * glyphBitmapHeight_;

    for (int32_t i=0; i<bakedGlyphs.size(); i++)
    {
        auto& baked = bakedGlyphs[i];

        X_ASSERT(bitMapSize * (i + 1) <= static_cast<size_t>(bakedData.length()), "Out of range")(bitMapSize * (i + 1), bakedData.length());
        const char* pBitMapSrc = bakedData.data() + (bitMapSize * i);

        // glyph.
        XGlyph* pSlot = GetLRUSlot();
        if (!pSlot) {
            return false;
        }

        pSlot->usage = usage;
        pSlot->currentChar = baked.currentChar;
        pSlot->charWidth = baked.charWidth;
        pSlot->charHeight = baked.charHeight;
        pSlot->charOffsetX = baked.charOffsetX;
        pSlot->charOffsetY = baked.charOffsetY;
        pSlot->bitmapOffsetX = baked.bitmapOffsetX;
        pSlot->bitmapOffsetY = baked.bitmapOffsetY;

        // fill in the bitmap.
        auto& bitmap = pSlot->glyphBitmap;
        auto& bitBuf = bitmap.GetBuffer();

        X_ASSERT(bitBuf.size() == bitMapSize, "Size mismatch")(bitBuf.size(), bitMapSize);
        std::memcpy(bitBuf.data(), pBitMapSrc, bitMapSize);

        cacheTable_.insert(std::make_pair(baked.currentChar, pSlot));
    }

    return true;
}


void XGlyphCache::GetGlyphBitmapSize(int32_t* pWidth, int32_t* pHeight) const
{
    if (pWidth) {
        *pWidth = glyphBitmapWidth_;
    }

    if (pHeight) {
        *pHeight = glyphBitmapHeight_;
    }
}

bool XGlyphCache::PreCacheGlyph(wchar_t cChar)
{
    X_ASSERT(cacheTable_.find(cChar) == cacheTable_.end(), "Precache caleed when already in cache")();

    // get the Least recently used Slot
    XGlyph* pSlot = GetLRUSlot();
    if (!pSlot) {
        X_WARNING("Font", "failed to find a free slot for: %i", cChar);
        return false;
    }

    // already used?
    if (pSlot->usage > 0) {
        UnCacheGlyph(pSlot->currentChar);
    }

    fontRenderer_.EnabledDebugRender(vars_.glyphCacheDebugRender());

    // scaling
    if (scaleBitmap_) {
        int32_t offsetMult = getOffsetMulti(smoothAmount_);

        scaleBitmap_->Clear();

        X_ASSERT_NOT_NULL(pSlot);
        X_ASSERT_NOT_NULL(scaleBitmap_.get());
        if (!fontRenderer_.GetGlyph(*pSlot, *scaleBitmap_, cChar, true)) {
            // failed to render
            return false;
        }

        pSlot->advanceX >>= offsetMult >> 1;
        pSlot->charWidth >>= offsetMult >> 1;
        pSlot->charHeight >>= offsetMult >> 1;
        pSlot->charOffsetX >>= offsetMult >> 1;
        pSlot->charOffsetY >>= offsetMult >> 1;

        // Hack: need to handle updating these values better so don't clip bits.
        const uint32_t paddingX = (pSlot->glyphBitmap.GetWidth() - pSlot->charWidth) / 2;
        const uint32_t paddingY = (pSlot->glyphBitmap.GetHeight() - pSlot->charHeight) / 2;

        pSlot->bitmapOffsetX = safe_static_cast<int8_t>(paddingX);
        pSlot->bitmapOffsetY = safe_static_cast<int8_t>(paddingY);
        pSlot->charWidth += 1;
        pSlot->charHeight += 1;
        // ~hack

        scaleBitmap_->BlitScaledTo8(
            pSlot->glyphBitmap.GetBuffer(),
            0,
            0,
            scaleBitmap_->GetWidth(),
            scaleBitmap_->GetHeight(),
            0,
            0,
            pSlot->glyphBitmap.GetWidth(),
            pSlot->glyphBitmap.GetHeight(),
            pSlot->glyphBitmap.GetWidth());
    }
    else {
        X_ASSERT_NOT_NULL(pSlot);
        if (!fontRenderer_.GetGlyph(*pSlot, pSlot->glyphBitmap, cChar, true)) {
            // failed to render
            return false;
        }
    }

    // Blur it baby!
    if (smoothMethod_ == FontSmooth::BLUR) {
        int32_t iterations = 1;

        switch (smoothAmount_) {
            case FontSmoothAmount::X2:
                iterations = 2;
                break;
            case FontSmoothAmount::X4:
                iterations = 4;
                break;
        }

        pSlot->glyphBitmap.Blur(iterations);
    }

    pSlot->usage = usage_;

    cacheTable_.insert(std::make_pair(cChar, pSlot));
    return true;
}

bool XGlyphCache::UnCacheGlyph(wchar_t cChar)
{
    XCacheTable::iterator pItor = cacheTable_.find(cChar);

    if (pItor != cacheTable_.end()) {
        XGlyph* pSlot = pItor->second;
        pSlot->reset();
        cacheTable_.erase(pItor);
        return true;
    }

    return false;
}

bool XGlyphCache::GlyphCached(wchar_t cChar)
{
    return (cacheTable_.find(cChar) != cacheTable_.end());
}

//-------------------------------------------------------------------------------------------------

XGlyph* XGlyphCache::GetLRUSlot(void)
{
    const auto it = std::min_element(slotList_.begin(), slotList_.end(), [](const XGlyph& s1, const XGlyph& s2) {
        return s1.usage < s2.usage;
    });

    auto& slot = *it;
    return &slot;
}

//-------------------------------------------------------------------------------------------------

XGlyph* XGlyphCache::GetMRUSlot(void)
{
    const auto it = std::max_element(slotList_.begin(), slotList_.end(), [](const XGlyph& s1, const XGlyph& s2) {
        return s1.usage < s2.usage;
    });

    auto& slot = *it;
    return &slot;
}

//-------------------------------------------------------------------------------------------------

const XGlyph* XGlyphCache::GetGlyph(wchar_t cChar)
{
    // glyph already chached?
    XCacheTable::iterator pItor = cacheTable_.find(cChar);
    if (pItor == cacheTable_.end()) {
        if (!PreCacheGlyph(cChar)) {
            X_ERROR("Font", "Failed to cache glyph for char: '%lc'", cChar);
            return nullptr;
        }

        pItor = cacheTable_.find(cChar);
    }

    // should be in the cache table now.
    X_ASSERT_NOT_NULL(pItor->second);
    auto* pGlyph = pItor->second;

    // update usage for LRU
    pGlyph->usage = usage_++;

    return pGlyph;
}

//-------------------------------------------------------------------------------------------------

bool XGlyphCache::CreateSlotList(size_t listSize)
{
    slotList_.reserve(listSize);

    for (size_t i = 0; i < listSize; i++) {
        XGlyph& slot = slotList_.AddOne(g_fontArena);
        slot.reset();
        slot.glyphBitmap.Create(glyphBitmapWidth_, glyphBitmapHeight_);
    }

    return true;
}

//-------------------------------------------------------------------------------------------------

void XGlyphCache::ReleaseSlotList(void)
{
    slotList_.free();
}

X_NAMESPACE_END
