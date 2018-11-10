#include "stdafx.h"
#include "Font.h"

#include "Sys\XFontSystem.h"
#include "FontRender\XFontTexture.h"

#include <IFileSys.h>
#include <Threading\JobSystem2.h>

#include <Memory\MemCursor.h>

X_NAMESPACE_BEGIN(font)

bool XFont::processData(core::UniquePointer<char[]> data, uint32_t dataSize)
{
    if (dataSize < sizeof(FontHdr)) {
        return false;
    }

    FontHdr& hdr = *reinterpret_cast<FontHdr*>(data.get());

    if (!hdr.isValid()) {
        X_ERROR("Font", "\"%s\" header is invalid", name_.c_str());
        return false;
    }

    if (hdr.numEffects < 1) {
        X_ERROR("Font", "\"%s\" has no effects", name_.c_str());
        return false;
    }

    core::MemCursor cursor(data.ptr() + sizeof(hdr), dataSize - sizeof(hdr));

    // each effect has multiple passes.
    effectsHdr_ = core::make_span(cursor.postSeekPtr<FontEffectHdr>(hdr.numEffects), hdr.numEffects);
    effectsPasses_ = core::make_span(cursor.postSeekPtr<FontPass>(hdr.numPasses), hdr.numPasses);

    // baked glphys hdr
    bakedGlyphs_ = core::make_span(cursor.postSeekPtr<GlyphHdr>(hdr.numGlyphs), hdr.numGlyphs);
    
    // baked glphys bitmaps
    auto bakedDataSize = (hdr.glyphWidth * hdr.glyphHeight) * hdr.numGlyphs;
    bakedData_ = core::make_span(cursor.postSeekPtr<char>(bakedDataSize), bakedDataSize);

    // src.
    X_ASSERT(hdr.sourceFontSize == cursor.numBytesRemaning(), "Font parse error")(hdr.sourceFontSize, cursor.numBytesRemaning());

    fontSrc_ = core::make_span(cursor.postSeekPtr<uint8_t>(hdr.sourceFontSize), hdr.sourceFontSize);

    // Make a font texture for it.
    pFontTexture_ = core::makeUnique<XFontTexture>(g_fontArena, fontSys_.getVars(), g_fontArena);

    // setup the font texture cpu buffers.
    if (!pFontTexture_->Create(512, 512, 16, 16, bakedGlyphs_, bakedData_, fontSrc_)) {
        X_ERROR("Font", "\"%s\" failed to create font texture", name_.c_str());
        return false;
    }

    if (fontSys_.getVars().glyphCachePreWarm()) {
        pFontTexture_->PreWarmCache();
    }

    data_ = std::move(data);
    return true;
}

X_NAMESPACE_END
