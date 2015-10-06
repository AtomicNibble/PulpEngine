#include "stdafx.h"
#include "XFontRender.h"
#include "XGlyphBitmap.h"


X_NAMESPACE_BEGIN(font)

XFontRender::XFontRender() : 
pLibrary_(0),
pFace_(0),
pGlyph_(0), 
pEncoding_(X_FONT_ENCODING_UNICODE),
fSizeRatio_(0.8f),
// fSizeRatio_(1.0f),
iGlyphBitmapWidth_(0),
iGlyphBitmapHeight_(0)
{

}

XFontRender::~XFontRender()
{
	Release();
}


bool XFontRender::Release()
{
	FT_Done_Face(pFace_);
	FT_Done_FreeType(pLibrary_);
	pFace_ = nullptr;
	pLibrary_ = nullptr;
	return true;
}

bool XFontRender::LoadFromMemory(BYTE* pBuffer, size_t bufferLength)
{
	int err = FT_Init_FreeType(&pLibrary_);

	if (err)
	{
		X_ERROR("Font", "failed to init freetype. Error: %i", err);
		return false;
	}

	if (pFace_)
	{
		FT_Done_Face(pFace_);
		pFace_ = 0;
	}

	err = FT_New_Memory_Face(
		pLibrary_, 
		pBuffer, 
		safe_static_cast<int,size_t>(bufferLength), 
		0, 
		&pFace_
		);

	if (err)
	{
		X_ERROR("Font", "failed to create new freetype face. Error: %i", err);
		return false;
	}

	SetEncoding(X_FONT_ENCODING_UNICODE);
	return true;
}


void XFontRender::SetGlyphBitmapSize(int width, int height)
{
	iGlyphBitmapWidth_ = width;
	iGlyphBitmapHeight_ = height;

	int err = FT_Set_Pixel_Sizes(pFace_, 
		(int)(iGlyphBitmapWidth_ * fSizeRatio_),
		(int)(iGlyphBitmapHeight_ * fSizeRatio_));

	if (err)
	{
		X_ERROR("Font", "failed to set pixel size(%i,%i). Error: %i", width, height, err);
	}
}

void XFontRender::GetGlyphBitmapSize(int* pWidth, int* pHeight) const
{
	if (pWidth)
		*pWidth = iGlyphBitmapWidth_;
	if (pHeight)
		*pHeight = iGlyphBitmapHeight_;
}


bool XFontRender::SetEncoding(FT_Encoding pEncoding)
{
	if (FT_Select_Charmap(pFace_, pEncoding)) {
		return false;
	}

	return true;
}


bool	XFontRender::GetGlyph(XGlyphBitmap *pGlyphBitmap, uint8 *iGlyphWidth, uint8 *iGlyphHeight,
		char &iCharOffsetX, char &iCharOffsetY, int iX, int iY, int iCharCode)
{ 
	int iError = FT_Load_Char(pFace_, iCharCode, FT_LOAD_DEFAULT);

	if (iError) {
		return false;
	}

	pGlyph_ = pFace_->glyph;

	iError = FT_Render_Glyph(pGlyph_, FT_RENDER_MODE_NORMAL);

	if (iError) {
		return false;
	}

	if (iGlyphWidth) {
		*iGlyphWidth = safe_static_cast<uint8_t, int>(pGlyph_->bitmap.width);
	}
	if (iGlyphHeight) {
		*iGlyphHeight = safe_static_cast<uint8_t, int>(pGlyph_->bitmap.rows);
	}

//	int iTopOffset = (iGlyphBitmapHeight_ - (int)(iGlyphBitmapHeight_ * fSizeRatio_)) + pGlyph_->bitmap_top;

	iCharOffsetX = (char)pGlyph_->bitmap_left;
	iCharOffsetY = (char)((int)(iGlyphBitmapHeight_ * fSizeRatio_) - pGlyph_->bitmap_top);		// is that correct? - we need the baseline

	unsigned char* pBuffer = pGlyphBitmap->GetBuffer();
	uint32 dwGlyphWidth = pGlyphBitmap->GetWidth();

	for (int i = 0; i < pGlyph_->bitmap.rows; i++)
	{
		int iNewY = i + iY;

		for (int j = 0; j < pGlyph_->bitmap.width; j++)
		{
			unsigned char	cColor = pGlyph_->bitmap.buffer[(i * pGlyph_->bitmap.width) + j];
			int				iOffset = iNewY * dwGlyphWidth + iX + j;

			if (iOffset >= (int)dwGlyphWidth * iGlyphBitmapHeight_)
				continue;

#if X_FONT_DEBUG_RENDER
			pBuffer[iOffset] = cColor / 2 + 32;
#else
			pBuffer[iOffset] = cColor;
#endif // !X_FONT_DEBUG_RENDER
		}
	}

	return true;
}


//------------------------------------------------------------------------------------------------- 

X_NAMESPACE_END