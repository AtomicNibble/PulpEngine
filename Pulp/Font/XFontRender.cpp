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


bool XFontRender::GetGlyph(XGlyphBitmap* pGlyphBitmap, uint8* pGlyphWidth, uint8* pGlyphHeight,
	int8_t& charOffsetX, int8_t& charOffsetY, int32_t iX, int32_t iY, int32_t charCode)
{ 
	int32_t err = FT_Load_Char(pFace_, charCode, FT_LOAD_DEFAULT);
	if (err) {
		X_ERROR("Font", "Failed to render glyp for char(%" PRIi32 "): '%lc'. Error: \"%s\"", err, charCode, errToStr(err));
		return false;
	}

	pGlyph_ = pFace_->glyph;

	err = FT_Render_Glyph(pGlyph_, FT_RENDER_MODE_NORMAL);
	if (err) {
		X_ERROR("Font", "Failed to render glyp for char(%" PRIi32 "): '%lc'. Error: \"%s\"", err, charCode, errToStr(err));
		return false;
	}

	if (pGlyphWidth) {
		*pGlyphWidth = safe_static_cast<uint8_t, int32_t>(pGlyph_->bitmap.width);
	}
	if (pGlyphHeight) {
		*pGlyphHeight = safe_static_cast<uint8_t, int32_t>(pGlyph_->bitmap.rows);
	}

	charOffsetX = safe_static_cast<int8_t>(pGlyph_->bitmap_left);
	charOffsetY = safe_static_cast<int8_t>(static_cast<uint32_t>(glyphBitmapHeight_ * sizeRatio_) - pGlyph_->bitmap_top);		// is that correct? - we need the baseline

	auto& buffer = pGlyphBitmap->GetBuffer();
	uint32 glyphWidth = pGlyphBitmap->GetWidth();

	for (uint32_t i = 0; i < pGlyph_->bitmap.rows; i++)
	{
		const int32_t newY = i + iY;

		for (uint32_t j = 0; j < pGlyph_->bitmap.width; j++)
		{
			uint8_t	color =		pGlyph_->bitmap.buffer[(i * pGlyph_->bitmap.width) + j];
			int32_t				offset = newY * glyphWidth + iX + j;

			if (offset >= static_cast<int32_t>(glyphWidth * glyphBitmapHeight_)) {
				continue;
			}

#if X_FONT_DEBUG_RENDER
			buffer[offset] = color / 2 + 64;
#else
			buffer[offset] = color;
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