#include "stdafx.h"
#include "XFontRender.h"
#include "XGlyphBitmap.h"


X_NAMESPACE_BEGIN(font)

XFontRender::XFontRender() : 
	pLibrary_(0),
	pFace_(0),
	pGlyph_(0), 
	pEncoding_(ENCODING_UNICODE),
	fSizeRatio_(0.8f),
	// fSizeRatio_(1.0f),
	glyphBitmapWidth_(0),
	glyphBitmapHeight_(0)
{

}

XFontRender::~XFontRender()
{
	Release();
}


bool XFontRender::Release(void)
{
	FT_Done_Face(pFace_);
	FT_Done_FreeType(pLibrary_);
	pFace_ = nullptr;
	pLibrary_ = nullptr;
	return true;
}

bool XFontRender::LoadFromMemory(const BufferArr& buf)
{
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
		buf.ptr(),
		safe_static_cast<int32_t>(buf.size()),
		0, 
		&pFace_
	);

	if (err)
	{
		X_ERROR("Font", "failed to create new freetype face. Error(%" PRIi32 "): \"%s\"", err, errToStr(err));
		return false;
	}

	SetEncoding(ENCODING_UNICODE);
	return true;
}


void XFontRender::SetGlyphBitmapSize(int32_t width, int32_t height)
{
	glyphBitmapWidth_ = width;
	glyphBitmapHeight_ = height;

	const int32_t err = FT_Set_Pixel_Sizes(pFace_, 
		static_cast<int32_t>(glyphBitmapWidth_ * fSizeRatio_),
		static_cast<int32_t>(glyphBitmapHeight_ * fSizeRatio_));

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


bool XFontRender::SetEncoding(FT_Encoding pEncoding)
{
	const int32_t err = FT_Select_Charmap(pFace_, pEncoding);	
	if(err) {
		X_ERROR("Font", "Failed to set encode to: %i. Error(%" PRIi32 "): \"%s\"", pEncoding, err, errToStr(err));
		return false;
	}

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
	charOffsetY = safe_static_cast<int8_t>(static_cast<uint32_t>(glyphBitmapHeight_ * fSizeRatio_) - pGlyph_->bitmap_top);		// is that correct? - we need the baseline

	uint8_t* pBuffer = pGlyphBitmap->GetBuffer();
	uint32 dwGlyphWidth = pGlyphBitmap->GetWidth();

	for (uint32_t i = 0; i < pGlyph_->bitmap.rows; i++)
	{
		const int32_t newY = i + iY;

		for (uint32_t j = 0; j < pGlyph_->bitmap.width; j++)
		{
			uint8_t	color =		pGlyph_->bitmap.buffer[(i * pGlyph_->bitmap.width) + j];
			int32_t				offset = newY * dwGlyphWidth + iX + j;

			if (offset >= static_cast<int32_t>(dwGlyphWidth * glyphBitmapHeight_)) {
				continue;
			}

#if X_FONT_DEBUG_RENDER
			pBuffer[offset] = color / 2 + 32;
#else
			pBuffer[offset] = color;
#endif // !X_FONT_DEBUG_RENDER
		}
	}

	return true;
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