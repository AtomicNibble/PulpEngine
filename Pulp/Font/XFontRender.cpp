#include "stdafx.h"
#include "XFontRender.h"
#include "XGlyphBitmap.h"


X_NAMESPACE_BEGIN(font)

XFontRender::XFontRender() : 
	pLibrary_(0),
	pFace_(0),
	pGlyph_(0), 
	encoding_(FontEncoding::Unicode),
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


bool XFontRender::GetGlyph(XGlyphBitmap& glyphBitmap, uint8* pGlyphWidth, uint8* pGlyphHeight,
	uint8_t& charOffsetX, uint8_t& charOffsetY, int32_t destOffsetX, int32_t destOffsetY, int32_t charCode)
{ 
	int32_t err = FT_Load_Char(pFace_, charCode, FT_LOAD_DEFAULT);
	if (err) {
		X_ERROR("Font", "Failed to render glyp for char: '%lc'. Error(%" PRIi32 "): \"%s\"", charCode, err, errToStr(err));
		return false;
	}

	pGlyph_ = pFace_->glyph;

	err = FT_Render_Glyph(pGlyph_, FT_RENDER_MODE_NORMAL);
	if (err) {
		X_ERROR("Font", "Failed to render glyp for char(%" PRIi32 "): '%lc'. Error(%" PRIi32 "): \"%s\"", charCode, err, errToStr(err));
		return false;
	}


	charOffsetX = safe_static_cast<uint8_t>(pGlyph_->bitmap_left);
	charOffsetY = safe_static_cast<uint8_t>(static_cast<uint32_t>(glyphBitmapHeight_ * sizeRatio_) - pGlyph_->bitmap_top);		// is that correct? - we need the baseline

	auto& buffer = glyphBitmap.GetBuffer();
	const uint32 dstGlyphWidth = glyphBitmap.GetWidth();
	const uint32 dstGlyphHeight = glyphBitmap.GetHeight();
	const uint32 maxIndex = dstGlyphWidth * dstGlyphHeight;

	// does the bitmap fit into the dest are the requested offset?
	if (dstGlyphWidth < (pGlyph_->bitmap.width + destOffsetX) || dstGlyphHeight < (pGlyph_->bitmap.rows + destOffsetY))
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


	if (pGlyphWidth) {
		auto cappedWidth = core::Min(dstGlyphWidth, pGlyph_->bitmap.width);
		*pGlyphWidth = safe_static_cast<uint8_t>(cappedWidth);
	}
	if (pGlyphHeight) {
		auto cappedHeight = core::Min(dstGlyphHeight, pGlyph_->bitmap.rows);
		*pGlyphHeight = safe_static_cast<uint8_t>(cappedHeight);
	}

	for (uint32_t row = 0; row < pGlyph_->bitmap.rows; row++)
	{
		const int32_t dstY = row + destOffsetY;

		for (uint32_t col = 0; col < pGlyph_->bitmap.width; col++)
		{
			const int32_t dstX = col + destOffsetX;
			const int32_t dstOffset = (dstY * dstGlyphWidth) + dstX;

			const int32_t srcOffset = (row * pGlyph_->bitmap.width) + col;
			const uint8_t srcColor = pGlyph_->bitmap.buffer[srcOffset];

			// overflow.
			if (dstOffset >= static_cast<int32_t>(buffer.size())) {
				continue;
			}

#if X_FONT_DEBUG_RENDER
			buffer[dstOffset] = srcColor / 2 + 64;
#else
			buffer[dstOffset] = srcColor;
#endif // !X_FONT_DEBUG_RENDER
		}
	}



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

	sizeRatio_ = sizeRatio;
	glyphBitmapWidth_ = width;
	glyphBitmapHeight_ = height;

	const int32_t err = FT_Set_Pixel_Sizes(
		pFace_,
		static_cast<int32_t>(glyphBitmapWidth_ * sizeRatio_),
		static_cast<int32_t>(glyphBitmapHeight_ * sizeRatio_)
	);

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