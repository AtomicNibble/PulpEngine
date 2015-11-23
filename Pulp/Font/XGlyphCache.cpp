#include "stdafx.h"
#include "XGlyphCache.h"

#include "XFontTexture.h"

X_NAMESPACE_BEGIN(font)


XGlyphCache::XGlyphCache() :

iGlyphBitmapWidth_(0),
iGlyphBitmapHeight_(0),
fSizeRatio_(0.8f),

pScaleBitmap_(nullptr),

uUsage_(0),

iSmoothMethod_(0),
iSmoothAmount_(0)
{

}

XGlyphCache::~XGlyphCache()
{

}

bool XGlyphCache::Create(int iCacheSize, int iGlyphBitmapWidth, int iGlyphBitmapHeight, 
						int iSmoothMethod, int iSmoothAmount, float fSizeRatio)
{
	fSizeRatio_ = fSizeRatio;

	iSmoothMethod_ = iSmoothMethod;
	iSmoothAmount_ = iSmoothAmount;

	iGlyphBitmapWidth_ = iGlyphBitmapWidth;
	iGlyphBitmapHeight_ = iGlyphBitmapHeight;

	if (!CreateSlotList(iCacheSize))
	{
		ReleaseSlotList();
		return false;
	}

	int iScaledGlyphWidth = 0;
	int iScaledGlyphHeight = 0;

	switch (iSmoothAmount_)
	{
		case FontSmooth::SUPERSAMPLE:
		{
			switch (iSmoothAmount_)
			{
				case FontSmoothAmount::X2:
					iScaledGlyphWidth = iGlyphBitmapWidth_ << 1;
					iScaledGlyphHeight = iGlyphBitmapHeight_ << 1;
				break;
				case FontSmoothAmount::X4:
					iScaledGlyphWidth = iGlyphBitmapWidth_ << 2;
					iScaledGlyphHeight = iGlyphBitmapHeight_ << 2;
				break;
			}
		}
		break;
		default:
			break;
	}

	// Scaled?
	if (iScaledGlyphWidth)
	{
		pScaleBitmap_ = X_NEW(XGlyphBitmap, g_fontArena,"BitMap");

		if (!pScaleBitmap_)
		{
			Release();
			return false;
		}

		if (!pScaleBitmap_->Create(iScaledGlyphWidth, iScaledGlyphHeight))
		{
			Release();
			return false;
		}

		FontRenderer_.SetGlyphBitmapSize(iScaledGlyphWidth, iScaledGlyphHeight);
	}
	else
	{
		FontRenderer_.SetGlyphBitmapSize(32, 32);
	}

	return true;
}

void XGlyphCache::Release()
{
	ReleaseSlotList();

	CacheTable_.clear();

	if (pScaleBitmap_)
	{
		pScaleBitmap_->Release();

		X_DELETE_AND_NULL(pScaleBitmap_, g_fontArena);
	}

	iGlyphBitmapWidth_ = 0;
	iGlyphBitmapHeight_ = 0;
	fSizeRatio_ = 0.8f;
}

void XGlyphCache::ReleaseFont()
{
	FontRenderer_.Release();
}

bool XGlyphCache::LoadFontFromMemory(unsigned char *pFileBuffer, size_t iDataSize)
{
	return FontRenderer_.LoadFromMemory(pFileBuffer, iDataSize);
}


void XGlyphCache::GetGlyphBitmapSize(int *pWidth, int *pHeight) const
{
	if (pWidth)
	{
		*pWidth = iGlyphBitmapWidth_;
	}

	if (pHeight)
	{
		*pHeight = iGlyphBitmapHeight_;
	}
}


bool XGlyphCache::PreCacheGlyph(wchar_t cChar)
{
	XCacheTable::iterator pItor = CacheTable_.find(cChar);

	// already chaced?
	if (pItor != CacheTable_.end())
	{
		pItor->second->uUsage = uUsage_;
		return true;
	}

	// get the Least recently used Slot
	XCacheSlot* pSlot = GetLRUSlot();
	if (!pSlot)
	{
		X_WARNING("Font", "failed to find a free slot for: %i", cChar);
		return false;
	}

	// already used?
	if (pSlot->uUsage > 0)
	{
		UnCacheGlyph(pSlot->cCurrentChar);
	}

	// scaling 
	if (pScaleBitmap_)
	{
		int iOffsetMult = 1;

		switch (iSmoothAmount_)
		{
			case FontSmoothAmount::X2:
			iOffsetMult = 2;
			break;
			case FontSmoothAmount::X4:
			iOffsetMult = 4;
			break;
		}

		pScaleBitmap_->Clear();

		if (!FontRenderer_.GetGlyph(pScaleBitmap_, &pSlot->iCharWidth, &pSlot->iCharHeight, 
				pSlot->iCharOffsetX, pSlot->iCharOffsetY, 0, 0, cChar))
		{
			// failed to render
			return false;
		}

		pSlot->iCharWidth >>= iOffsetMult >> 1;
		pSlot->iCharHeight >>= iOffsetMult >> 1;

		pScaleBitmap_->BlitScaledTo8(
			pSlot->pGlyphBitmap.GetBuffer(),
			0, 0, 
			pScaleBitmap_->GetWidth(), 
			pScaleBitmap_->GetHeight(),
			0, 0, 
			pSlot->pGlyphBitmap.GetWidth(), 
			pSlot->pGlyphBitmap.GetHeight(), 
			pSlot->pGlyphBitmap.GetWidth());
	}
	else
	{
		if (!FontRenderer_.GetGlyph(&pSlot->pGlyphBitmap, &pSlot->iCharWidth, &pSlot->iCharHeight,
			pSlot->iCharOffsetX, pSlot->iCharOffsetY, 0, 0, cChar))
		{
			// failed to render
			return false;
		}
	}

	// Blur it baby!
	if (iSmoothMethod_ == FontSmooth::BLUR)
	{
		pSlot->pGlyphBitmap.Blur(iSmoothAmount_);
	}

	pSlot->uUsage = uUsage_;
	pSlot->cCurrentChar = cChar;

	CacheTable_.insert(std::pair<wchar_t, XCacheSlot *>(cChar, pSlot));
	return true;
}

bool XGlyphCache::UnCacheGlyph(wchar_t cChar)
{
	XCacheTable::iterator pItor = CacheTable_.find(cChar);

	if (pItor != CacheTable_.end())
	{
		XCacheSlot *pSlot = pItor->second;
		pSlot->Reset();
		CacheTable_.erase(pItor);
		return true;
	}

	return false;
}

bool XGlyphCache::GlyphCached(wchar_t cChar)
{
	return (CacheTable_.find(cChar) != CacheTable_.end());
}

//------------------------------------------------------------------------------------------------- 

XCacheSlot* XGlyphCache::GetLRUSlot()
{
	unsigned int	dwMinUsage = 0xffffffff;
	XCacheSlot		*pLRUSlot = 0;
	XCacheSlot		*pSlot;

	XCacheSlotListItor pItor = SlotList_.begin();

	while (pItor != SlotList_.end())
	{
		pSlot = *pItor;
		if (pSlot->uUsage == 0)
		{
			return pSlot;
		}
		else
		{
			if (pSlot->uUsage < dwMinUsage)
			{
				pLRUSlot = pSlot;
				dwMinUsage = pSlot->uUsage;
			}
		}
		++pItor;
	}
	return pLRUSlot;
}

//------------------------------------------------------------------------------------------------- 

XCacheSlot* XGlyphCache::GetMRUSlot()
{
	unsigned int	dwMaxUsage = 0;
	XCacheSlot		*pMRUSlot = 0;
	XCacheSlot		*pSlot;

	XCacheSlotListItor pItor = SlotList_.begin();

	while (pItor != SlotList_.end())
	{
		pSlot = *pItor;

		if (pSlot->uUsage != 0)
		{
			if (pSlot->uUsage > dwMaxUsage)
			{
				pMRUSlot = pSlot;
				dwMaxUsage = pSlot->uUsage;
			}
		}
		++pItor;
	}
	return pMRUSlot;
}

//------------------------------------------------------------------------------------------------- 

bool XGlyphCache::GetGlyph(XGlyphBitmap **pGlyph, int *piWidth, int *piHeight, char &iCharOffsetX, char &iCharOffsetY, wchar_t cChar)
{
	XCacheTable::iterator pItor = CacheTable_.find(cChar);

	// glyph already chached?
	if (pItor == CacheTable_.end())
	{
		if (!PreCacheGlyph(cChar))
		{
			return false;
		}
	}

	// should be in the cache table now.
	pItor = CacheTable_.find(cChar);

	pItor->second->uUsage = uUsage_++;
	(*pGlyph) = &pItor->second->pGlyphBitmap;

	if (piWidth) {
		*piWidth = pItor->second->iCharWidth;
	}

	if (piHeight) {
		*piHeight = pItor->second->iCharHeight;
	}

	iCharOffsetX = pItor->second->iCharOffsetX;
	iCharOffsetY = pItor->second->iCharOffsetY;

	return true;
}

//------------------------------------------------------------------------------------------------- 

bool XGlyphCache::CreateSlotList(int listSize)
{
	for (int i = 0; i < listSize; i++)
	{
		XCacheSlot* pCacheSlot = X_NEW(XCacheSlot,g_fontArena, "GlyphCache");

		if (!pCacheSlot) {
			return false;
		}

		if (!pCacheSlot->pGlyphBitmap.Create(iGlyphBitmapWidth_, iGlyphBitmapHeight_))
		{
			X_DELETE(pCacheSlot,g_fontArena);
			return false;
		}

		pCacheSlot->Reset();
		pCacheSlot->iCacheSlot = i;

		SlotList_.push_back(pCacheSlot);
	}

	return true;
}

//------------------------------------------------------------------------------------------------- 

void XGlyphCache::ReleaseSlotList()
{
	XCacheSlotListItor pItor = SlotList_.begin();

	while (pItor != SlotList_.end())
	{
		(*pItor)->pGlyphBitmap.Release();
		X_DELETE( (*pItor), g_fontArena);
		pItor = SlotList_.erase(pItor);
	}
}



X_NAMESPACE_END
