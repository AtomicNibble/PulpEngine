#include "stdafx.h"
#include "XGlyphCache.h"

#include "XFontTexture.h"

X_NAMESPACE_BEGIN(font)


XGlyphCache::XGlyphCache(core::MemoryArenaBase* arena) :

iGlyphBitmapWidth_(0),
iGlyphBitmapHeight_(0),
fSizeRatio_(0.8f),

pScaleBitmap_(nullptr),

uUsage_(0),

iSmoothMethod_(0),
iSmoothAmount_(0),

SlotList_(arena),
CacheTable_(arena, 8)
{

}

XGlyphCache::~XGlyphCache()
{

}

bool XGlyphCache::Create(int32_t iCacheSize, int32_t iGlyphBitmapWidth, int32_t iGlyphBitmapHeight,
	int32_t iSmoothMethod, int32_t iSmoothAmount, float fSizeRatio)
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

	int32_t iScaledGlyphWidth = 0;
	int32_t iScaledGlyphHeight = 0;

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
		pScaleBitmap_ = X_NEW(XGlyphBitmap, arena_,"BitMap");

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

		fontRenderer_.SetGlyphBitmapSize(iScaledGlyphWidth, iScaledGlyphHeight);
	}
	else
	{
		fontRenderer_.SetGlyphBitmapSize(32, 32);
	}

	return true;
}

void XGlyphCache::Release(void)
{
	ReleaseSlotList();

	CacheTable_.clear();

	if (pScaleBitmap_)
	{
		pScaleBitmap_->Release();

		X_DELETE_AND_NULL(pScaleBitmap_, arena_);
	}

	iGlyphBitmapWidth_ = 0;
	iGlyphBitmapHeight_ = 0;
	fSizeRatio_ = 0.8f;
}

void XGlyphCache::ReleaseFont(void)
{
	fontRenderer_.Release();
}

bool XGlyphCache::LoadFontFromMemory(uint8_t* pFileBuffer, size_t iDataSize)
{
	return fontRenderer_.LoadFromMemory(pFileBuffer, iDataSize);
}


void XGlyphCache::GetGlyphBitmapSize(int32_t* pWidth, int32_t* pHeight) const
{
	if (pWidth) {
		*pWidth = iGlyphBitmapWidth_;
	}

	if (pHeight) {
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

		if (!fontRenderer_.GetGlyph(pScaleBitmap_, &pSlot->iCharWidth, &pSlot->iCharHeight,
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
		if (!fontRenderer_.GetGlyph(&pSlot->pGlyphBitmap, &pSlot->iCharWidth, &pSlot->iCharHeight,
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
		pSlot->reset();
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

XCacheSlot* XGlyphCache::GetMRUSlot(void)
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

bool XGlyphCache::GetGlyph(XGlyphBitmap** pGlyph, int32_t* piWidth, int32_t* piHeight, 
	int8_t& iCharOffsetX, int8_t& iCharOffsetY, wchar_t cChar)
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

bool XGlyphCache::CreateSlotList(size_t listSize)
{
	for (size_t i = 0; i < listSize; i++)
	{
		XCacheSlot* pCacheSlot = X_NEW(XCacheSlot, arena_, "GlyphCache");

		if (!pCacheSlot) {
			return false;
		}

		if (!pCacheSlot->pGlyphBitmap.Create(iGlyphBitmapWidth_, iGlyphBitmapHeight_))
		{
			X_DELETE(pCacheSlot, arena_);
			return false;
		}

		pCacheSlot->reset();
		pCacheSlot->iCacheSlot = i;

		SlotList_.push_back(pCacheSlot);
	}

	return true;
}

//------------------------------------------------------------------------------------------------- 

void XGlyphCache::ReleaseSlotList(void)
{
	XCacheSlotListItor pItor = SlotList_.begin();

	while (pItor != SlotList_.end())
	{
		XCacheSlot* pSlot = (*pItor);
		++pItor;

		pSlot->pGlyphBitmap.Release();
		X_DELETE((*pItor), arena_);
	}

	SlotList_.free();
}



X_NAMESPACE_END
