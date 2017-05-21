#include "stdafx.h"
#include "XFontRender.h"
#include "XFontGlyph.h"

X_NAMESPACE_BEGIN(font)

XFontRender::XFontRender() : 
	pLibrary_(nullptr),
	pFace_(nullptr),
	encoding_(FontEncoding::Unicode),
	debugRender_(false),
	sizeRatio_(0.8f),
	glyphBitmapWidth_(0),
	glyphBitmapHeight_(0),
	data_(g_fontArena)
{

}

XFontRender::~XFontRender()
{
	Release();
}


bool XFontRender::SetRawFontBuffer(core::UniquePointer<uint8_t[]> data, int32_t length, FontEncoding::Enum encoding)
{
	data_ = std::move(data);

	int32_t err = FT_Init_FreeType(&pLibrary_);
	if (err)
	{
		X_ERROR("Font", "failed to init freetype. Error(%" PRIi32 "): \"%s\"", err, errToStr(err));
		return false;
	}

	if (pFace_)
	{
		FT_Done_Face(pFace_);
		pFace_ = nullptr;
	}

	err = FT_New_Memory_Face(
		pLibrary_, 
		data_.ptr(),
		length,
		0, 
		&pFace_
	);

	if (err)
	{
		X_ERROR("Font", "failed to create new freetype face. Error(%" PRIi32 "): \"%s\"", err, errToStr(err));
		return false;
	}

	SetEncoding(encoding);
	return true;
}


bool XFontRender::Release(void)
{
	FT_Done_Face(pFace_);
	FT_Done_FreeType(pLibrary_);
	pFace_ = nullptr;
	pLibrary_ = nullptr;

	data_.reset();
	return true;
}


bool XFontRender::GetGlyph(XGlyph& glphy, XGlyphBitmap& destBitMap, int32_t destOffsetX, int32_t destOffsetY, wchar_t charCode)
{ 
	int32_t err = FT_Load_Char(pFace_, static_cast<FT_ULong>(charCode), FT_LOAD_DEFAULT);
	if (err) {
		X_ERROR("Font", "Failed to render glyp for char: '%lc'. Error(%" PRIi32 "): \"%s\"", charCode, err, errToStr(err));
		return false;
	}

	FT_GlyphSlot pGlyph = pFace_->glyph;

	err = FT_Render_Glyph(pGlyph, FT_RENDER_MODE_NORMAL);
	if (err) {
		X_ERROR("Font", "Failed to render glyp for char(%" PRIi32 "): '%lc'. Error(%" PRIi32 "): \"%s\"", charCode, err, errToStr(err));
		return false;
	}


	auto& buffer = destBitMap.GetBuffer();
	const uint32 dstGlyphWidth = destBitMap.GetWidth();
	const uint32 dstGlyphHeight = destBitMap.GetHeight();
	const uint32 maxIndex = dstGlyphWidth * dstGlyphHeight;

#if X_DEBUG

	// make sure we not relying on zero initialize.
	// since i only copy pixels i have.
	std::memset(buffer.data(), 0xFF, buffer.size());

	// some sanity checks for my understanding of FreeType.
	// they are not very accurate out by few pixels in some cases so keep disabled when not testing.
#if 0
	{
		const auto& sizeInfo = pFace_->size->metrics;

		// the top of a font should not be above a ascender.
		X_ASSERT(pGlyph->bitmap_top <= (sizeInfo.ascender / 64) + 1, "Unxpected bitmap top")(sizeInfo.ascender, sizeInfo.ascender / 64, pGlyph->bitmap_top);
		// the top + height should not be lowest than descender.
		// aka if we have 64height with a top of 1 and 5 rows.
		// we have a descender of -4
		const auto bitmapBottom = pGlyph->bitmap_top - safe_static_cast<int32_t>(pGlyph->bitmap.rows);
		X_ASSERT(bitmapBottom >= (sizeInfo.descender / 64) - 1, "Bitmap end is lower than descender")(sizeInfo.descender, sizeInfo.descender / 64, bitmapBottom);
		
		// check advance
		const auto advance = pGlyph->advance;
		X_ASSERT(advance.x <= static_cast<float>(sizeInfo.max_advance + 64), "Advance was greater than expected")(sizeInfo.max_advance, pGlyph->advance);
		// is the width + left less than max_advance?
	}
#endif

#endif // !X_DEBUG

	// does the bitmap fit into the dest are the requested offset?
	if (dstGlyphWidth < (pGlyph->bitmap.width + destOffsetX) || dstGlyphHeight < (pGlyph->bitmap.rows + destOffsetY))
	{
		if (destOffsetX || destOffsetY)
		{
			X_WARNING("Font", "Glyph for char '%lc' does not fit in dest bimap at offsets %" PRIi32 ", %" PRIi32 ", clipping.",
				charCode, destOffsetX, destOffsetY);
		}
		else
		{
			X_WARNING("Font", "Glyph for char '%lc' does not fit in dest bimap clipping.",charCode);
		}
	}

	const uint32_t colsToCopy = core::Min(dstGlyphWidth, destOffsetX + pGlyph->bitmap.width);
	const uint32_t rowsToCopy = core::Min(dstGlyphHeight, destOffsetY + pGlyph->bitmap.rows);

	for (uint32_t row = 0; row < rowsToCopy; row++)
	{
		const int32_t dstX = destOffsetX;
		const int32_t dstY = row + destOffsetY;
		const int32_t dstOffset = (dstY * dstGlyphWidth) + dstX;

		const int32_t srcOffset = (row * pGlyph->bitmap.pitch);

		uint8_t* pDstRow = &buffer[dstOffset];
		const uint8_t* pSrcRow = &pGlyph->bitmap.buffer[srcOffset];

		std::memcpy(pDstRow, pSrcRow, colsToCopy);
	}

	if (debugRender_) {
		for (auto& p : buffer) {
			p = p / 2 + 64;
		}
	}


	// the top is like from the pen.
	// so in order to make this a Y offset it's the size minus top.
	const auto offsetY = static_cast<uint32_t>(glyphBitmapHeight_ - pGlyph->bitmap_top);

	glphy.currentChar = charCode;
	glphy.advanceX = safe_static_cast<uint16_t>(pGlyph->advance.x / 64);
	glphy.charOffsetX = safe_static_cast<decltype(glphy.charOffsetX)>(pGlyph->bitmap_left);
	glphy.charOffsetY = safe_static_cast<decltype(glphy.charOffsetX)>(offsetY);
	glphy.charWidth = safe_static_cast<uint8_t>(colsToCopy);
	glphy.charHeight = safe_static_cast<uint8_t>(rowsToCopy);
	return true;
}



bool XFontRender::SetEncoding(FontEncoding::Enum encoding)
{
	FT_Encoding ftEncoding = FT_ENCODING_UNICODE;

	static_assert(FontEncoding::ENUM_COUNT == 2, "More encoding types? this logic needs updating");

	encoding_ = encoding;
	switch (encoding) 
	{
		case FontEncoding::Unicode:
			ftEncoding = FT_ENCODING_UNICODE;
			break;
		case FontEncoding::MSSymbol:
			ftEncoding = FT_ENCODING_MS_SYMBOL;
			break;
		default:
			X_ASSERT_UNREACHABLE();
			break;
	}

	const int32_t err = FT_Select_Charmap(pFace_, ftEncoding);
	if (err) {
		X_ERROR("Font", "Failed to set encode to: %s. Error(%" PRIi32 "): \"%s\"", FontEncoding::ToString(encoding), err, errToStr(err));
		return false;
	}

	return true;
}


void XFontRender::SetGlyphBitmapSize(int32_t width, int32_t height, float sizeRatio)
{
	X_ASSERT(width > 0 && height > 0, "Width and height must be none zero")(width, height);
	X_ASSERT(sizeRatio > 0.f && sizeRatio <= 1.f, "Size ratio invalid")(sizeRatio);

	sizeRatio_ = sizeRatio;
	glyphBitmapWidth_ = width;
	glyphBitmapHeight_ = height;

	const auto scaledWidth = static_cast<int32_t>(glyphBitmapWidth_ * sizeRatio_); 
	const auto scaledHeight = static_cast<int32_t>(glyphBitmapHeight_ * sizeRatio_);

	const int32_t err = FT_Set_Pixel_Sizes(
		pFace_,
		scaledWidth,
		scaledHeight
	);

	metrics_.ascender = pFace_->size->metrics.ascender / 64;
	metrics_.descender = pFace_->size->metrics.descender / 64;
	metrics_.max_advance = pFace_->size->metrics.max_advance / 64;

	if (err)
	{
		X_ERROR("Font", "failed to set pixel size(%i,%i). Error(%" PRIi32 "): \"%s\"", width, height, err, errToStr(err));
	}
}

void XFontRender::GetGlyphBitmapSize(int32_t* pWidth, int32_t* pHeight) const
{
	if (pWidth) {
		*pWidth = glyphBitmapWidth_;
	}
	if (pHeight) {
		*pHeight = glyphBitmapHeight_;
	}
}


//------------------------------------------------------------------------------------------------- 

const char* XFontRender::errToStr(FT_Error err)
{
	#undef FTERRORS_H_
    #define FT_ERRORDEF( e, v, s )  case e: return s;
    #define FT_ERROR_START_LIST     switch (err) {
    #define FT_ERROR_END_LIST       }
    #include FT_ERRORS_H
    return "<ukn>";
}

X_NAMESPACE_END