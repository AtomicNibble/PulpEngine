#include "stdafx.h"
#include "XFontTexture.h"

#include "Vars\FontVars.h"

#include <IFileSys.h>
#include <Threading\JobSystem2.h>

X_NAMESPACE_BEGIN(font)

XFontTexture::XFontTexture(const FontVars& vars, core::MemoryArenaBase* arena) :
    glyphCache_(vars, arena),
    textureSlotArea_(arena),

    width_(0),
    height_(0),
    textureBuffer_(arena),

    invWidth_(0),
    invHeight_(0),

    cellWidth_(0),
    cellHeight_(0),

    textureCellWidth_(0),
    textureCellHeight_(0),

    widthCellCount_(0),
    heightCellCount_(0),

    slotUsage_(1), // space for gradiant.
    cacheMisses_(0),

    slotList_(arena),
    slotTable_(arena, 8)
{
}

XFontTexture::~XFontTexture()
{
}

void XFontTexture::Clear(void)
{
    textureBuffer_.clear();

    width_ = 0;
    height_ = 0;
    invWidth_ = 0;
    invHeight_ = 0;
    cellWidth_ = 0;
    cellHeight_ = 0;
    textureCellWidth_ = 0;
    textureCellHeight_ = 0;
    widthCellCount_ = 0;
    heightCellCount_ = 0;

    slotUsage_ = 1;
    cacheMisses_ = 0;

    ReleaseSlotList();
}

bool XFontTexture::Create(int32_t width, int32_t height, int32_t widthCellCount, int32_t heightCellCount,
    core::span<const GlyphHdr> bakedGlyphs, core::span<const char> bakedData, core::span<const uint8_t> fontSrc)
{
    if (!core::bitUtil::IsPowerOfTwo(width) || !core::bitUtil::IsPowerOfTwo(height)) {
        X_ERROR("Font", "Font texture must be pow2. width: %" PRIi32 " height: %" PRIi32, width, height);
        return false;
    }

    textureBuffer_.resize(width * height);
    std::fill(textureBuffer_.begin(), textureBuffer_.end(), 0_ui8);

    width_ = width;
    height_ = height;
    invWidth_ = 1.0f / static_cast<float>(width);
    invHeight_ = 1.0f / static_cast<float>(height);

    widthCellCount_ = widthCellCount;
    heightCellCount_ = heightCellCount;

    cellWidth_ = width / widthCellCount;
    cellHeight_ = height / heightCellCount;

    textureCellWidth_ = cellWidth_ * invWidth_;
    textureCellHeight_ = cellHeight_ * invHeight_;

    if (!glyphCache_.Create(cellWidth_, cellHeight_)) {
        Clear();
        return false;
    }

    int32_t textureSlotCount = widthCellCount * heightCellCount;
    if (!CreateSlotList(textureSlotCount)) {
        Clear();
        return false;
    }

    CreateGradientSlot();

    if (!glyphCache_.SetRawFontBuffer(fontSrc, FontEncoding::Unicode)) {
        Clear();
        return false;
    }

    if (!glyphCache_.SetBakedData(bakedGlyphs, bakedData)) {
        Clear();
        return false;
    }

    return true;
}

void XFontTexture::PreWarmCache(void)
{
    X_PROFILE_NO_HISTORY_BEGIN("FontWarmCache", core::profiler::SubSys::FONT);

    size_t len = X_ARRAY_SIZE(FONT_PRECACHE_STR) - 1;
    len = core::Min(len, slotList_.size()); // only precache what we can fit in the cache.

    X_ASSERT(len > 0, "Cache must not be zero in size")(slotList_.size());

    PreCacheString(FONT_PRECACHE_STR, FONT_PRECACHE_STR + len);
}

CacheResult::Enum XFontTexture::PreCacheString(const wchar_t* pBegin, const wchar_t* pEnd, int32_t* pUpdatedOut)
{
    uint16 slotUsage = slotUsage_++;
    size_t length = union_cast<size_t>(pEnd - pBegin);
    int32_t updated = 0;

    X_ASSERT(pEnd >= pBegin, "Invalid begin end pair")(pBegin, pEnd);

    for (size_t i = 0; i < length; i++) {
        const wchar_t cChar = pBegin[i];

        XTextureSlot* pSlot = GetCharSlot(cChar);
        if (!pSlot) {
            ++cacheMisses_;

            // get a free slot.
            pSlot = GetLRUSlot();
            if (!pSlot) {
                X_ERROR("Font", "Failed to get free slot for char");
                return CacheResult::ERROR;
            }

            if (!UpdateSlot(pSlot, slotUsage, cChar)) {
                return CacheResult::ERROR;
            }

            ++updated;
        }
        else {
            // update the LRU vale
            pSlot->slotUsage = slotUsage;
        }
    }

    if (pUpdatedOut) {
        *pUpdatedOut = updated;
    }

    if (updated > 0) {
        return CacheResult::UPDATED;
    }

    return CacheResult::UNCHANGED;
}

//-------------------------------------------------------------------------------------------------
wchar_t XFontTexture::GetSlotChar(int32_t slot) const
{
    return slotList_[slot].currentChar;
}

//-------------------------------------------------------------------------------------------------
XTextureSlot* XFontTexture::GetCharSlot(wchar_t cChar)
{
    XTextureSlotTableItor pItor = slotTable_.find(cChar);

    if (pItor != slotTable_.end()) {
        return pItor->second;
    }

    return nullptr;
}

//-------------------------------------------------------------------------------------------------
XTextureSlot* XFontTexture::GetGradientSlot(void)
{
    X_ASSERT(slotList_.isNotEmpty(), "slot list should be valid")();
    return &slotList_[0];
}

//-------------------------------------------------------------------------------------------------
XTextureSlot* XFontTexture::GetLRUSlot(void)
{
    const auto it = std::min_element(slotList_.begin(), slotList_.end(), [](const XTextureSlot& s1, const XTextureSlot& s2) {
        return s1.slotUsage < s2.slotUsage;
    });

    auto& slot = *it;
    return &slot;
}

//-------------------------------------------------------------------------------------------------
XTextureSlot* XFontTexture::GetMRUSlot(void)
{
    const auto it = std::max_element(slotList_.begin(), slotList_.end(), [](const XTextureSlot& s1, const XTextureSlot& s2) {
        return s1.slotUsage < s2.slotUsage;
    });

    auto& slot = *it;
    return &slot;
}

bool XFontTexture::CreateSlotList(int32_t listSize)
{
    int32_t y, x;

    slotList_.resize(listSize);

    const float xbias = (0.5f / static_cast<float>(width_));
    const float ybias = (0.5f / static_cast<float>(height_));

    for (int32_t i = 0; i < listSize; i++) {
        XTextureSlot& slot = slotList_[i];

        slot.textureSlot = i;
        slot.reset();

        y = i / widthCellCount_;
        x = i % widthCellCount_;

        float u = static_cast<float>(x) * textureCellWidth_;
        float v = static_cast<float>(y) * textureCellHeight_;

        slot.texCoord[0] = u;
        slot.texCoord[1] = v;
    }

    return true;
}

//-------------------------------------------------------------------------------------------------
bool XFontTexture::ReleaseSlotList(void)
{
    slotList_.free();
    return true;
}

//-------------------------------------------------------------------------------------------------
bool XFontTexture::UpdateSlot(XTextureSlot* pSlot, uint16 slotUsage, wchar_t cChar)
{
    X_ASSERT_NOT_NULL(pSlot);

    // remove and replace.
    XTextureSlotTableItor pItor = slotTable_.find(pSlot->currentChar);
    if (pItor != slotTable_.end()) {
        slotTable_.erase(pItor);
    }

    slotTable_.insert(std::make_pair(cChar, pSlot));

    pSlot->slotUsage = slotUsage;
    pSlot->currentChar = cChar;

    auto* pGlyph = glyphCache_.GetGlyph(cChar);

    if (!pGlyph) {
        X_ERROR("Font", "Failed to get glyph for char: '%lc'", cChar);
        return false;
    }

    pSlot->charWidth = pGlyph->charWidth;
    pSlot->charHeight = pGlyph->charHeight;
    pSlot->charOffsetX = safe_static_cast<decltype(pSlot->charOffsetX)>(pGlyph->charOffsetX);
    pSlot->charOffsetY = safe_static_cast<decltype(pSlot->charOffsetY)>(pGlyph->charOffsetY);
    pSlot->paddingX = pGlyph->bitmapOffsetX;
    pSlot->paddingY = pGlyph->bitmapOffsetY;
    pSlot->advanceX = pGlyph->advanceX;
    //	pSlot->advanceY = 0;

    const int32_t slotBufferX = pSlot->textureSlot % widthCellCount_;
    const int32_t slotBufferY = pSlot->textureSlot / widthCellCount_;

    // blit the glyp to my buffer
    // always copy the whole buffer, we may of blended.
    pGlyph->glyphBitmap.BlitTo8(
        textureBuffer_.ptr(),
        0,                         // srcX
        0,                         // srcY
        cellWidth_,                // srcWidth
        cellHeight_,               // srcHeight
        slotBufferX * cellWidth_,  // destx
        slotBufferY * cellHeight_, // desy
        width_                     // destWidth / stride
    );

    return true;
}

void XFontTexture::CreateGradientSlot(void)
{
    XTextureSlot* pSlot = GetGradientSlot();

    // 0 needs to be unused spot
    X_ASSERT(pSlot->currentChar == static_cast<wchar_t>(~0), "slot idx zero needs to be empty")(pSlot->currentChar);

    pSlot->reset();
    pSlot->charWidth = safe_static_cast<uint8_t, int32_t>(cellWidth_ - 2);
    pSlot->charHeight = safe_static_cast<uint8_t, int32_t>(cellHeight_ - 2);
    pSlot->setNotReusable();

    const int32_t x = pSlot->textureSlot % widthCellCount_;
    const int32_t y = pSlot->textureSlot / widthCellCount_;

    uint8* pBuffer = &textureBuffer_[x * cellWidth_ + y * cellHeight_ * height_];

    for (uint32 dwY = 0; dwY < pSlot->charHeight; ++dwY) {
        for (uint32 dwX = 0; dwX < pSlot->charWidth; ++dwX) {
            pBuffer[dwX + dwY * width_] = safe_static_cast<uint8_t, uint32_t>(
                dwY * 255 / (pSlot->charHeight - 1));
        }
    }
}

bool XFontTexture::WriteToFile(const char* filename)
{
    core::XFileScoped file;
    core::Path<char> path;
    BITMAPFILEHEADER pHeader;
    BITMAPINFOHEADER pInfoHeader;

    path = "Fonts/";
    path.setFileName(filename);
    path.setExtension(".bmp");

    if (textureBuffer_.isEmpty()) {
        X_WARNING("Font", "Failed to write font texture, buffer is invalid.");
        return false;
    }

    if (file.openFile(path, core::FileFlag::RECREATE | core::FileFlag::WRITE)) {
        core::zero_object(pHeader);
        core::zero_object(pInfoHeader);

        pHeader.bfType = 0x4D42;
        pHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + width_ * height_ * 3;
        pHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

        pInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
        pInfoHeader.biWidth = width_;
        pInfoHeader.biHeight = height_;
        pInfoHeader.biPlanes = 1;
        pInfoHeader.biBitCount = 24;
        pInfoHeader.biCompression = 0;
        pInfoHeader.biSizeImage = width_ * height_ * 3;

        file.write(pHeader);
        file.write(pInfoHeader);

        uint8_t cRGB[3];

        core::ByteStream stream(g_fontArena);

        stream.reserve(height_ * width_ * 3);

        for (int32_t i = height_ - 1; i >= 0; i--) {
            for (int32_t j = 0; j < width_; j++) {
                cRGB[0] = textureBuffer_[(i * width_) + j];
                cRGB[1] = *cRGB;
                cRGB[2] = *cRGB;

                stream.write(cRGB, sizeof(cRGB));
            }
        }

        file.write(stream.begin(), stream.size());
        return true;
    }

    return false;
}

//-------------------------------------------------------------------------------------------------

void XFontTexture::GetTextureCoord(const XTextureSlot* pSlot, XCharCords& cords) const
{
    X_ASSERT_NOT_NULL(pSlot);

    const int32_t chWidth = pSlot->charWidth;
    const int32_t chHeight = pSlot->charHeight;
    float slotCoord0 = pSlot->texCoord[0];
    float slotCoord1 = pSlot->texCoord[1];

    slotCoord0 += static_cast<float>(pSlot->paddingX * invWidth_);
    slotCoord1 += static_cast<float>(pSlot->paddingY * invHeight_);

    // i need to offset corrds by padding.
    cords.texCoords[0] = slotCoord0;
    cords.texCoords[1] = slotCoord1;
    cords.texCoords[2] = slotCoord0 + static_cast<float>(chWidth * invWidth_);
    cords.texCoords[3] = slotCoord1 + static_cast<float>(chHeight * invHeight_);

    cords.size[0] = chWidth;
    cords.size[1] = chHeight;
    cords.offset[0] = pSlot->charOffsetX;
    cords.offset[1] = pSlot->charOffsetY;
}

X_NAMESPACE_END