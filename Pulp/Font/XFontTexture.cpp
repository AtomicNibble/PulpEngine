#include "stdafx.h"
#include "XFontTexture.h"

#include <IFileSys.h>

X_NAMESPACE_BEGIN(font)

XFontTexture::XFontTexture(core::MemoryArenaBase* arena) :
width_(0),
height_(0),
pBuffer_(nullptr),

invWidth_(0),
invHeight_(0),

cellWidth_(0),
cellHeight_(0),

textureCellWidth_(0),
textureCellHeight_(0),

widthCellCount_(0),
heightCellCount_(0),

textureSlotCount_(0),

smoothMethod_(0),
smoothAmount_(0),

slotUsage_(1), // space for gradiant.

glyphCache_(arena),
slotList_(arena),
slotTable_(arena)
{

}

XFontTexture::~XFontTexture()
{
	Release();
}


int32_t XFontTexture::Release()
{
	X_DELETE_ARRAY(pBuffer_, g_fontArena);
	pBuffer_ = nullptr;

	width_ = 0;
	height_ = 0;

	ReleaseSlotList();

	glyphCache_.Release();

	return 1;
}

bool XFontTexture::CreateFromMemory(BYTE* pFileData, size_t dataLength, int32_t iWidth,
	int32_t iHeight, int32_t smoothMethod, int32_t smoothAmount,
	float fSizeRatio, int32_t iWidthCharCount, int32_t iHeightCharCount)
{
	if (!glyphCache_.LoadFontFromMemory(pFileData, dataLength))
	{
		Release();
		return false;
	}

	if (!Create(iWidth, iHeight, smoothMethod, smoothAmount,
		fSizeRatio, iWidthCharCount, iHeightCharCount)) {
		return false;
	}

	return true;
}

bool XFontTexture::Create(int32_t iWidth, int32_t iHeight, int32_t smoothMethod, int32_t smoothAmount,
	float fSizeRatio, int32_t iWidthCellCount, int32_t iHeightCellCount)
{
	pBuffer_ = X_NEW_ARRAY(uint8, iWidth * iHeight, g_fontArena, "fontTexture");
	if (!pBuffer_) {
		return false;
	}

	std::memset(pBuffer_, 0, iWidth * iHeight * sizeof(uint8));
	
	width_ = iWidth;
	height_ = iHeight;
	invWidth_ = 1.0f / static_cast<float>(iWidth);
	invHeight_ = 1.0f / static_cast<float>(iHeight);

	widthCellCount_ = iWidthCellCount;
	heightCellCount_ = iHeightCellCount;
	textureSlotCount_ = iWidthCellCount * iHeightCellCount;

#if 0
	smoothMethod_ = X_FONT_SMOOTH_SUPERSAMPLE;
	smoothAmount_ = X_FONT_SMOOTH_AMOUNT_4X;

#else
	smoothMethod_ = smoothMethod;
	smoothAmount_ = smoothAmount;
#endif

	cellWidth_ = iWidth / iWidthCellCount;
	cellHeight_ = iHeight / iHeightCellCount;

	textureCellWidth_ = cellWidth_ * invWidth_;
	textureCellHeight_ = cellHeight_ * invHeight_;

	
	if (!glyphCache_.Create(X_FONT_GLYPH_CACHE_SIZE,
		cellWidth_, cellHeight_, 
		smoothMethod_, smoothAmount_, 
		fSizeRatio))
	{
		Release();
		return false;
	}

	if (!CreateSlotList(textureSlotCount_))
	{
		Release();
		return false;
	}

	return true;
}


int32_t XFontTexture::PreCacheString(const wchar_t* szString, int32_t* pUpdated)
{
	uint16 wSlotUsage = slotUsage_++;
	size_t length = core::strUtil::strlen(szString);
	size_t iUpdated = 0;

	for (size_t i = 0; i < length; i++)
	{
		wchar_t cChar = szString[i];

		XTextureSlot* pSlot = GetCharSlot(cChar);

		if (!pSlot)
		{
			pSlot = GetLRUSlot();

			if (!pSlot) {
				return 0;
			}

			if (!UpdateSlot(pSlot->textureSlot, wSlotUsage, cChar)) {
				return 0;
			}

			++iUpdated;
		}
		else
		{
			pSlot->slotUsage = wSlotUsage;
		}
	}

	if (pUpdated) {
		*pUpdated = iUpdated;
	}

	if (iUpdated) {
		return 1;
	}

	return 2;
}

//-------------------------------------------------------------------------------------------------
wchar_t XFontTexture::GetSlotChar(int32_t slot) const
{
	return slotList_[slot]->currentChar;
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
XTextureSlot* XFontTexture::GetGradientSlot()
{
	return slotList_[0];
}

//-------------------------------------------------------------------------------------------------
XTextureSlot* XFontTexture::GetLRUSlot()
{
	uint16 wMinSlotUsage = 0xffff;
	XTextureSlot* pLRUSlot = nullptr;
	XTextureSlot* pSlot;

	XTextureSlotListItor pItor = slotList_.begin();

	while (pItor != slotList_.end())
	{
		pSlot = *pItor;

		if (pSlot->slotUsage == 0)
		{
			return pSlot;
		}
		else
		{
			if (pSlot->slotUsage  < wMinSlotUsage)
			{
				pLRUSlot = pSlot;
				wMinSlotUsage = pSlot->slotUsage;
			}
		}

		pItor++;
	}

	return pLRUSlot;
}

//-------------------------------------------------------------------------------------------------
XTextureSlot *XFontTexture::GetMRUSlot(void)
{
	uint16 wMaxSlotUsage = 0;
	XTextureSlot *pMRUSlot = 0;
	XTextureSlot *pSlot;

	XTextureSlotListItor pItor = slotList_.begin();

	while (pItor != slotList_.end())
	{
		pSlot = *pItor;

		if (pSlot->slotUsage != 0)
		{
			if (pSlot->slotUsage  > wMaxSlotUsage)
			{
				pMRUSlot = pSlot;
				wMaxSlotUsage = pSlot->slotUsage;
			}
		}

		pItor++;
	}

	return pMRUSlot;
}


int32_t XFontTexture::CreateSlotList(int32_t iListSize)
{
	int32_t y, x;

	for (int32_t i = 0; i < iListSize; i++)
	{
		XTextureSlot* pTextureSlot = X_NEW(XTextureSlot, g_fontArena,"fontTexSlot");

		if (!pTextureSlot) {
			return 0;
		}

		pTextureSlot->textureSlot = i;
		pTextureSlot->reset();

		y = i / widthCellCount_;
		x = i % widthCellCount_;

		pTextureSlot->texCoord[0] = static_cast<float>((x * textureCellWidth_) + (0.5f / static_cast<float>(width_)));
		pTextureSlot->texCoord[1] = static_cast<float>((y * textureCellHeight_) + (0.5f / static_cast<float>(height_)));

		// TODO: crashes in super dynamic x64 O.o
		slotList_.push_back(pTextureSlot);
	}

	return 1;
}

//-------------------------------------------------------------------------------------------------
int32_t XFontTexture::ReleaseSlotList(void)
{
	XTextureSlotListItor pItor = slotList_.begin();

	for (; pItor != slotList_.end(); ++pItor)
	{
		X_DELETE((*pItor), g_fontArena);
	}

	slotList_.free();
	return 1;
}

//-------------------------------------------------------------------------------------------------
int32_t XFontTexture::UpdateSlot(int32_t slot, uint16 wSlotUsage, wchar_t cChar)
{
	XTextureSlot* pSlot = slotList_[slot];

	if (!pSlot) {
		return 0;
	}

	XTextureSlotTableItor pItor = slotTable_.find(pSlot->currentChar);

	if (pItor != slotTable_.end()) {
		slotTable_.erase(pItor);
	}

	slotTable_.insert(std::pair<wchar_t, XTextureSlot*>(cChar, pSlot));

	pSlot->slotUsage = wSlotUsage;
	pSlot->currentChar = cChar;

	int32_t iWidth = 0;
	int32_t iHeight = 0;

	// blit the char glyph into the texture
	const int32_t x = pSlot->textureSlot % widthCellCount_;
	const int32_t y = pSlot->textureSlot / widthCellCount_;

	XGlyphBitmap *pGlyphBitmap;

	if (!glyphCache_.GetGlyph(&pGlyphBitmap,
		&iWidth, &iHeight,
		pSlot->charOffsetX, pSlot->charOffsetY,
		cChar)) {
		return 0;
	}

	pSlot->charWidth = safe_static_cast<uint8_t, int32_t>(iWidth);
	pSlot->charHeight = safe_static_cast<uint8_t, int32_t>(iHeight);

	pGlyphBitmap->BlitTo8(pBuffer_, 
		0, 0,
		iWidth, iHeight, 
		x * cellWidth_, y * cellHeight_, 
		width_);

	return 1;
}

void XFontTexture::CreateGradientSlot()
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

	uint8* pBuffer = &pBuffer_[x*cellWidth_ + y*cellHeight_*height_];

	for (uint32 dwY = 0; dwY < pSlot->charHeight; ++dwY) {
		for (uint32 dwX = 0; dwX < pSlot->charWidth; ++dwX) {
			pBuffer[dwX + dwY*width_] = safe_static_cast<uint8_t, uint32_t>(
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

	path /= "Fonts/";
	path.setFileName(filename);
	path.setExtension(".bmp");

	if (!pBuffer_) {
		X_WARNING("Font", "Failed to write font texture, buffer is invalid.");
		return false;
	}

	if (file.openFile(path.c_str(), core::fileMode::RECREATE | core::fileMode::WRITE))
	{
		core::zero_object(pHeader);
		core::zero_object(pInfoHeader);

		pHeader.bfType = 0x4D42;
		pHeader.bfSize = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+width_ * height_ * 3;
		pHeader.bfOffBits = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);

		pInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
		pInfoHeader.biWidth = width_;
		pInfoHeader.biHeight = height_;
		pInfoHeader.biPlanes = 1;
		pInfoHeader.biBitCount = 24;
		pInfoHeader.biCompression = 0;
		pInfoHeader.biSizeImage = width_ * height_ * 3;

		file.write(pHeader); 
		file.write(pInfoHeader);

		unsigned char cRGB[3];

		core::ByteStream stream(g_fontArena);

		stream.resize(height_ * width_* 3);

		for (int32_t i = height_ - 1; i >= 0; i--)
		{
			for (int32_t j = 0; j < width_; j++)
			{
				cRGB[0] = pBuffer_[(i * width_) + j];
				cRGB[1] = *cRGB;
				cRGB[2] = *cRGB;

				stream.write(cRGB[0]);
				stream.write(cRGB[1]);
				stream.write(cRGB[2]);
			}
		}

		file.write(stream.begin(), stream.size());
		return true;
	}
	
	return false;
}

//-------------------------------------------------------------------------------------------------


int32_t XFontTexture::GetCharacterWidth(wchar_t cChar) const
{
	XTextureSlotTableItorConst pItor = slotTable_.find(cChar);

	if (pItor == slotTable_.end()) {
		return 0;
	}

	const XTextureSlot &rSlot = *pItor->second;

	return rSlot.charWidth + 1; // extra pixel for nicer bilinear filter
}

void XFontTexture::GetTextureCoord(XTextureSlot* pSlot, XCharCords& cords) const
{
	if (!pSlot) {
		return;	// expected behavior
	}

	const int32_t chWidth = pSlot->charWidth;
	const int32_t chHeight = pSlot->charHeight;
	const float slotCoord0 = pSlot->texCoord[0];
	const float slotCoord1 = pSlot->texCoord[1];


	cords.texCoords[0] = slotCoord0 - invWidth_;		// extra pixel for nicer bilinear filter
	cords.texCoords[1] = slotCoord1 - invHeight_;		// extra pixel for nicer bilinear filter
	cords.texCoords[2] = slotCoord0 + static_cast<float>(chWidth * invWidth_);
	cords.texCoords[3] = slotCoord1 + static_cast<float>(chHeight * invHeight_);

	cords.size[0] = chWidth + 1;		// extra pixel for nicer bilinear filter
	cords.size[1] = chHeight + 1;		// extra pixel for nicer bilinear filter
	cords.offset[0] = pSlot->charOffsetX;
	cords.offset[1] = pSlot->charOffsetY;
}


X_NAMESPACE_END