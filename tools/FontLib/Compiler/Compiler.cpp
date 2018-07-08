#include "stdafx.h"
#include "Compiler.h"

#include "FontRender\XFontGlyph.h"
#include "FontRender\XGlyphBitmap.h"

#include <IFileSys.h>

X_NAMESPACE_BEGIN(font)

FontCompiler::FontCompiler(core::MemoryArenaBase* arena) :
    arena_(arena),
    glyphs_(arena),
    effects_(arena),
    sourceFontData_(arena),
    render_(arena)
{
}

FontCompiler::~FontCompiler()
{
}

bool FontCompiler::setFont(DataVec&& trueTypeData, int32_t width, int32_t height, float sizeRatio)
{
    int32_t length = safe_static_cast<int32_t>(trueTypeData.size());

    auto data = core::makeUnique<uint8_t[]>(arena_, trueTypeData.size());
    std::memcpy(data.ptr(), trueTypeData.data(), trueTypeData.size());

    if (!render_.SetRawFontBuffer(std::move(data), length, FontEncoding::Unicode)) {
        X_ERROR("Font", "Failed to load font data");
        return false;
    }

    render_.SetGlyphBitmapSize(width, height, sizeRatio);

    sourceFontData_ = std::move(trueTypeData);
    return true;
}

bool FontCompiler::bake(bool sdf)
{
    const size_t len = X_ARRAY_SIZE(FONT_PRECACHE_STR) - 1;

    flags_.Clear();

    if (sdf) {
        flags_.Set(FontFlag::SDF);
    }

    glyphs_.reserve(len);
    glyphs_.clear();

    int32_t width, height;
    render_.GetGlyphBitmapSize(&width, &height);

    for (size_t i = 0; i < len; i++) {
        XGlyph& slot = glyphs_.AddOne(arena_);
        slot.reset();
        slot.glyphBitmap.Create(width, height);
        slot.currentChar = FONT_PRECACHE_STR[i];
    }

    X_LOG0("Font", "Generating glyphs...");

    for (size_t i = 0; i < glyphs_.size(); i++) {
        auto& glyph = glyphs_[i];

        if (!render_.GetGlyph(glyph, glyph.glyphBitmap, glyph.currentChar, true)) {
            X_ERROR("Font", "Failed to create glyph");
            return false;
        }
    }

    return true;
}

bool FontCompiler::writeToFile(core::XFile* pFile) const
{
    FontHdr hdr;
    core::zero_object(hdr);

    int32_t width, height;
    render_.GetGlyphBitmapSize(&width, &height);

    hdr.forcc = FontHdr::X_FONT_BIN_FOURCC;
    hdr.version = FontHdr::X_FONT_BIN_VERSION;
    hdr.flags = flags_;
    hdr.numEffects = safe_static_cast<uint8_t>(effects_.size());
    hdr.modifed = core::DateTimeStampSmall::systemDateTime();
    hdr.numGlyphs = safe_static_cast<uint16_t>(glyphs_.size());
    hdr.glyphWidth = safe_static_cast<uint16_t>(width);
    hdr.glyphHeight = safe_static_cast<uint16_t>(height);
    hdr.metrics = render_.GetMetrics();
    hdr.sourceFontSize = safe_static_cast<uint32_t>(sourceFontData_.size());

    // what do i want the format tobe?
    // basically I need all the info for each glyph.

    size_t dataSize = (sizeof(GlyphHdr) + (width * height)) * glyphs_.size();

    X_ASSERT(hdr.glyphDataSize() == dataSize, "Data size calc error")(hdr.glyphDataSize(), dataSize);

    dataSize += sizeof(hdr);
    dataSize += sourceFontData_.size();

    for (const auto& eff : effects_)
    {
        dataSize += sizeof(FontEffectHdr);
        dataSize += eff.passes.size() * sizeof(FontPass);
    }

    core::ByteStream stream(arena_);
    stream.reserve(dataSize);

    stream.write(hdr);

    for (const auto& efx : effects_)
    {
        if (efx.passes.isEmpty()) {
            X_ERROR("Font", "Effect has zero passes");
            return false;
        }

        FontEffectHdr efxHdr;
        efxHdr.numPass = safe_static_cast<uint8_t>(efx.passes.size());

        stream.write(efxHdr);
        stream.write(efx.passes.data(), efx.passes.size());
    }

    for (const auto& glyph : glyphs_)
    {
        auto& bitmap = glyph.glyphBitmap.GetBuffer();

        GlyphHdr glyphHdr;
        core::zero_object(glyphHdr);
        glyphHdr.currentChar = glyph.currentChar;
        glyphHdr.advanceX = glyph.advanceX;
        glyphHdr.charWidth = glyph.charWidth;
        glyphHdr.charHeight = glyph.charHeight;
        glyphHdr.charOffsetX = glyph.charOffsetX;
        glyphHdr.charOffsetY = glyph.charOffsetY;
        glyphHdr.bitmapOffsetX = glyph.bitmapOffsetX;
        glyphHdr.bitmapOffsetY = glyph.bitmapOffsetY;

        stream.write(glyphHdr);
        stream.write(bitmap.data(), bitmap.size());
    }

    stream.write(sourceFontData_.data(), sourceFontData_.size());

    X_ASSERT(stream.size() == dataSize, "Datasize mismatch")(stream.size(), dataSize); 

    if (pFile->write(stream.data(), stream.size()) != stream.size()) {
        X_ERROR("Font", "Failed to write data");
        return false;
    }

    return true;
}

bool FontCompiler::writeImageToFile(core::XFile* pFile) const
{
    BITMAPFILEHEADER header;
    BITMAPINFOHEADER infoHeader;

    core::zero_object(header);
    core::zero_object(infoHeader);

    int32_t numGlyphs = safe_static_cast<int32_t>(glyphs_.size());

    int32_t width = 32;
    int32_t height = 32;
    int32_t perRow = 0;

    int32_t glyphWidth, glyphHeight;
    render_.GetGlyphBitmapSize(&glyphWidth, &glyphHeight);

    {
        int32_t totalPixelsWide = numGlyphs * (glyphWidth * glyphHeight);

        while (1) {
            int32_t space = (width * height);
            if (space >= totalPixelsWide) {
                break;
            }

            if (width == height) {
                height <<= 1;
            }
            else {
                width <<= 1;
            }
        }

        perRow = width / glyphWidth;
    }

    const int32_t dataBytes = width * height * 3;

    header.bfType = 0x4D42;
    header.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dataBytes;
    header.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = width;
    infoHeader.biHeight = -height; // top down plz.
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 24;
    infoHeader.biCompression = 0;
    infoHeader.biSizeImage = dataBytes;

    DataVec imgData(arena_, dataBytes);
    std::fill(imgData.begin(), imgData.end(), 0);

    // copy in the bitmaps :)
    {
        for (int32_t i = 0; i < numGlyphs; i++) {
            const int32_t slotX = (i % perRow) * glyphWidth;
            const int32_t slotY = (i / perRow) * glyphHeight;

            auto& glyph = glyphs_[i];
            auto& bitmap = glyph.glyphBitmap;

            // i want to blit the bitmap into 24bit texture.
            bitmap.BlitTo24(
                imgData.data(),
                0,
                0,
                glyphWidth,
                glyphHeight,
                slotX,
                slotY,
                width);
        }
    }

    if (pFile->writeObj(header) != sizeof(header)) {
        X_ERROR("Font", "Failed to write header");
        return false;
    }

    if (pFile->writeObj(infoHeader) != sizeof(infoHeader)) {
        X_ERROR("Font", "Failed to write info header");
        return false;
    }

    if (pFile->write(imgData.data(), imgData.size()) != imgData.size()) {
        X_ERROR("Font", "Failed to write data");
        return false;
    }

    return true;
}

X_NAMESPACE_END