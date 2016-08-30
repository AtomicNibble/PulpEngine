#include "stdafx.h"
#include "XFontTexture.h"

#include <IFileSys.h>

X_NAMESPACE_BEGIN(font)

XFontTexture::XFontTexture(core::MemoryArenaBase* arena) :
width_(0),
height_(0),
pBuffer_(nullptr),

fInvWidth_(0),
fInvHeight_(0),

iCellWidth_(0),
iCellHeight_(0),

fTextureCellWidth_(0),
fTextureCellHeight_(0),

iWidthCellCount_(0),
iHeightCellCount_(0),

nTextureSlotCount_(0),

iSmoothMethod_(0),
iSmoothAmount_(0),

wSlotUsage_(1), // space for gradiant.

glyphCache_(arena),
slotList_(arena),
slotTable_(arena)
{

}

XFontTexture::~XFontTexture()
{
	Release();
}


int XFontTexture::Release()
{
	X_DELETE_ARRAY(pBuffer_, g_fontArena);
	pBuffer_ = nullptr;

	width_ = 0;
	height_ = 0;

	ReleaseSlotList();

	glyphCache_.Release();

	return 1;
}

bool XFontTexture::CreateFromMemory(BYTE* pFileData, size_t dataLength, int iWidth,
	int iHeight, int iSmoothMethod, int iSmoothAmount,
	float fSizeRatio, int iWidthCharCount, int iHeightCharCount)
{
	if (!glyphCache_.LoadFontFromMemory(pFileData, dataLength))
	{
		Release();
		return false;
	}

	if (!Create(iWidth, iHeight, iSmoothMethod, iSmoothAmount,
		fSizeRatio, iWidthCharCount, iHeightCharCount)) {
		return false;
	}

	return true;
}

bool XFontTexture::Create(int iWidth, int iHeight, int iSmoothMethod, int iSmoothAmount,
	float fSizeRatio, int iWidthCellCount, int iHeightCellCount)
{
	pBuffer_ = X_NEW_ARRAY(uint8, iWidth * iHeight, g_fontArena, "fontTexture");
	if (!pBuffer_) {
		return false;
	}

	std::memset(pBuffer_, 0, iWidth * iHeight * sizeof(uint8));
	
	width_ = iWidth;
	height_ = iHeight;
	fInvWidth_ = 1.0f / static_cast<float>(iWidth);
	fInvHeight_ = 1.0f / static_cast<float>(iHeight);

	iWidthCellCount_ = iWidthCellCount;
	iHeightCellCount_ = iHeightCellCount;
	nTextureSlotCount_ = iWidthCellCount * iHeightCellCount;

#if 0
	iSmoothMethod_ = X_FONT_SMOOTH_SUPERSAMPLE;
	iSmoothAmount_ = X_FONT_SMOOTH_AMOUNT_4X;

#else
	iSmoothMethod_ = iSmoothMethod;
	iSmoothAmount_ = iSmoothAmount;
#endif

	iCellWidth_ = iWidth / iWidthCellCount;
	iCellHeight_ = iHeight / iHeightCellCount;

	fTextureCellWidth_ = iCellWidth_ * fInvWidth_;
	fTextureCellHeight_ = iCellHeight_ * fInvHeight_;

	
	if (!glyphCache_.Create(X_FONT_GLYPH_CACHE_SIZE,
		iCellWidth_, iCellHeight_, 
		iSmoothMethod_, iSmoothAmount_, 
		fSizeRatio))
	{
		Release();
		return false;
	}

	if (!CreateSlotList(nTextureSlotCount_))
	{
		Release();
		return false;
	}

	return true;
}


int XFontTexture::PreCacheString(const wchar_t* szString, int* pUpdated)
{
	uint16 wSlotUsage = wSlotUsage_++;
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

			if (!UpdateSlot(pSlot->iTextureSlot, wSlotUsage, cChar)) {
				return 0;
			}

			++iUpdated;
		}
		else
		{
			pSlot->wSlotUsage = wSlotUsage;
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
wchar_t XFontTexture::GetSlotChar(int slot) const
{
	return slotList_[slot]->cCurrentChar;
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

		if (pSlot->wSlotUsage == 0)
		{
			return pSlot;
		}
		else
		{
			if (pSlot->wSlotUsage < wMinSlotUsage)
			{
				pLRUSlot = pSlot;
				wMinSlotUsage = pSlot->wSlotUsage;
			}
		}

		pItor++;
	}

	return pLRUSlot;
}

//-------------------------------------------------------------------------------------------------
XTextureSlot *XFontTexture::GetMRUSlot()
{
	uint16 wMaxSlotUsage = 0;
	XTextureSlot *pMRUSlot = 0;
	XTextureSlot *pSlot;

	XTextureSlotListItor pItor = slotList_.begin();

	while (pItor != slotList_.end())
	{
		pSlot = *pItor;

		if (pSlot->wSlotUsage != 0)
		{
			if (pSlot->wSlotUsage > wMaxSlotUsage)
			{
				pMRUSlot = pSlot;
				wMaxSlotUsage = pSlot->wSlotUsage;
			}
		}

		pItor++;
	}

	return pMRUSlot;
}


int XFontTexture::CreateSlotList(int iListSize)
{
	int32_t y, x;

	for (int i = 0; i < iListSize; i++)
	{
		XTextureSlot* pTextureSlot = X_NEW(XTextureSlot, g_fontArena,"fontTexSlot");

		if (!pTextureSlot) {
			return 0;
		}

		pTextureSlot->iTextureSlot = i;
		pTextureSlot->reset();

		y = i / iWidthCellCount_;
		x = i % iWidthCellCount_;

		pTextureSlot->vTexCoord[0] = static_cast<float>((x * fTextureCellWidth_) + (0.5f / static_cast<float>(width_)));
		pTextureSlot->vTexCoord[1] = static_cast<float>((y * fTextureCellHeight_) + (0.5f / static_cast<float>(height_)));

		// TODO: crashes in super dynamic x64 O.o
		slotList_.push_back(pTextureSlot);
	}

	return 1;
}

//-------------------------------------------------------------------------------------------------
int XFontTexture::ReleaseSlotList(void)
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
int XFontTexture::UpdateSlot(int iSlot, uint16 wSlotUsage, wchar_t cChar)
{
	XTextureSlot* pSlot = slotList_[iSlot];

	if (!pSlot) {
		return 0;
	}

	XTextureSlotTableItor pItor = slotTable_.find(pSlot->cCurrentChar);

	if (pItor != slotTable_.end()) {
		slotTable_.erase(pItor);
	}

	slotTable_.insert(std::pair<wchar_t, XTextureSlot*>(cChar, pSlot));

	pSlot->wSlotUsage = wSlotUsage;
	pSlot->cCurrentChar = cChar;

	int iWidth = 0;
	int iHeight = 0;

	// blit the char glyph into the texture
	const int32_t x = pSlot->iTextureSlot % iWidthCellCount_;
	const int32_t y = pSlot->iTextureSlot / iWidthCellCount_;

	XGlyphBitmap *pGlyphBitmap;

	if (!glyphCache_.GetGlyph(&pGlyphBitmap,
		&iWidth, &iHeight,
		pSlot->iCharOffsetX, pSlot->iCharOffsetY,
		cChar)) {
		return 0;
	}

	pSlot->iCharWidth = safe_static_cast<uint8_t, int>(iWidth);
	pSlot->iCharHeight = safe_static_cast<uint8_t, int>(iHeight);

	pGlyphBitmap->BlitTo8(pBuffer_, 
		0, 0,
		iWidth, iHeight, 
		x * iCellWidth_, y * iCellHeight_, 
		width_);

	return 1;
}

void XFontTexture::CreateGradientSlot()
{
	XTextureSlot* pSlot = GetGradientSlot();				
	
	// 0 needs to be unused spot
	X_ASSERT(pSlot->cCurrentChar == (uint16)~0, "slot idx zero needs to be empty")();		

	pSlot->reset();
	pSlot->iCharWidth = safe_static_cast<uint8_t, int32_t>(iCellWidth_ - 2);
	pSlot->iCharHeight = safe_static_cast<uint8_t, int32_t>(iCellHeight_ - 2);
	pSlot->setNotReusable();

	const int32_t x = pSlot->iTextureSlot % iWidthCellCount_;
	const int32_t y = pSlot->iTextureSlot / iWidthCellCount_;

	uint8* pBuffer = &pBuffer_[x*iCellWidth_ + y*iCellHeight_*height_];

	for (uint32 dwY = 0; dwY < pSlot->iCharHeight; ++dwY) {
		for (uint32 dwX = 0; dwX < pSlot->iCharWidth; ++dwX) {
			pBuffer[dwX + dwY*width_] = safe_static_cast<uint8_t, uint32_t>(
				dwY * 255 / (pSlot->iCharHeight - 1));
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

		for (int i = height_ - 1; i >= 0; i--)
		{
			for (int j = 0; j < width_; j++)
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


int XFontTexture::GetCharacterWidth(wchar_t cChar) const
{
	XTextureSlotTableItorConst pItor = slotTable_.find(cChar);

	if (pItor == slotTable_.end()) {
		return 0;
	}

	const XTextureSlot &rSlot = *pItor->second;

	return rSlot.iCharWidth + 1; // extra pixel for nicer bilinear filter
}

void XFontTexture::GetTextureCoord(XTextureSlot* pSlot, XCharCords& cords) const
{
	if (!pSlot) {
		return;	// expected behavior
	}

	const int32_t iChWidth = pSlot->iCharWidth;
	const int32_t iChHeight = pSlot->iCharHeight;
	const float slotCoord0 = pSlot->vTexCoord[0];
	const float slotCoord1 = pSlot->vTexCoord[1];


	cords.texCoords[0] = slotCoord0 - fInvWidth_;		// extra pixel for nicer bilinear filter
	cords.texCoords[1] = slotCoord1 - fInvHeight_;		// extra pixel for nicer bilinear filter
	cords.texCoords[2] = slotCoord0 + static_cast<float>(iChWidth * fInvWidth_);
	cords.texCoords[3] = slotCoord1 + static_cast<float>(iChHeight * fInvHeight_);

	cords.size[0] = iChWidth + 1;		// extra pixel for nicer bilinear filter
	cords.size[1] = iChHeight + 1;		// extra pixel for nicer bilinear filter
	cords.offset[0] = pSlot->iCharOffsetX;
	cords.offset[1] = pSlot->iCharOffsetY;
}


X_NAMESPACE_END