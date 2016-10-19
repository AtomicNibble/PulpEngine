#include "stdafx.h"
#include "XGlyphCache.h"

#include "XFontTexture.h"

X_NAMESPACE_BEGIN(font)


XGlyphCache::XGlyphCache(core::MemoryArenaBase* arena) :

arena_(arena),

glyphBitmapWidth_(0),
glyphBitmapHeight_(0),
fSizeRatio_(0.8f),

pScaleBitmap_(nullptr),

usage_(0),

smoothMethod_(0),
smoothAmount_(0),

slotList_(arena),
cacheTable_(arena, 8)
{

}

XGlyphCache::~XGlyphCache()
{

}

bool XGlyphCache::Create(int32_t cacheSize, int32_t glyphBitmapWidth, int32_t glyphBitmapHeight,
	int32_t smoothMethod, int32_t smoothAmount, float sizeRatio)
{
	fSizeRatio_ = sizeRatio;

	smoothMethod_ = smoothMethod;
	smoothAmount_ = smoothAmount;

	glyphBitmapWidth_ = glyphBitmapWidth;
	glyphBitmapHeight_ = glyphBitmapHeight;

	if (!CreateSlotList(cacheSize))
	{
		ReleaseSlotList();
		return false;
	}

	int32_t scaledGlyphWidth = 0;
	int32_t scaledGlyphHeight = 0;

	switch (smoothAmount_)
	{
		case FontSmooth::SUPERSAMPLE:
		{
			switch (smoothAmount_)
			{
				case FontSmoothAmount::X2:
					scaledGlyphWidth = glyphBitmapWidth_ << 1;
					scaledGlyphHeight = glyphBitmapHeight_ << 1;
				break;
				case FontSmoothAmount::X4:
					scaledGlyphWidth = glyphBitmapWidth_ << 2;
					scaledGlyphHeight = glyphBitmapHeight_ << 2;
				break;
			}
		}
		break;
		default:
			break;
	}

	// Scaled?
	if (scaledGlyphWidth)
	{
		pScaleBitmap_ = X_NEW(XGlyphBitmap, arena_,"BitMap");

		if (!pScaleBitmap_)
		{
			Release();
			return false;
		}

		if (!pScaleBitmap_->Create(scaledGlyphWidth, scaledGlyphHeight))
		{
			Release();
			return false;
		}

		fontRenderer_.SetGlyphBitmapSize(scaledGlyphWidth, scaledGlyphHeight);
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

	cacheTable_.clear();

	if (pScaleBitmap_)
	{
		pScaleBitmap_->Release();

		X_DELETE_AND_NULL(pScaleBitmap_, arena_);
	}

	glyphBitmapWidth_ = 0;
	glyphBitmapHeight_ = 0;
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
		*pWidth = glyphBitmapWidth_;
	}

	if (pHeight) {
		*pHeight = glyphBitmapHeight_;
	}
}


bool XGlyphCache::PreCacheGlyph(wchar_t cChar)
{
	XCacheTable::iterator pItor = cacheTable_.find(cChar);

	// already chaced?
	if (pItor != cacheTable_.end())
	{
		pItor->second->usage = usage_;
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
	if (pSlot->usage > 0)
	{
		UnCacheGlyph(pSlot->currentChar);
	}

	// scaling 
	if (pScaleBitmap_)
	{
		int32_t offsetMult = 1;

		switch (smoothAmount_)
		{
			case FontSmoothAmount::X2:
				offsetMult = 2;
			break;
			case FontSmoothAmount::X4:
				offsetMult = 4;
			break;
		}

		pScaleBitmap_->Clear();

		if (!fontRenderer_.GetGlyph(pScaleBitmap_, &pSlot->charWidth, &pSlot->charHeight,
				pSlot->charOffsetX, pSlot->charOffsetY, 0, 0, cChar))
		{
			// failed to render
			return false;
		}

		pSlot->charWidth >>= offsetMult >> 1;
		pSlot->charHeight >>= offsetMult >> 1;

		pScaleBitmap_->BlitScaledTo8(
			pSlot->glyphBitmap.GetBuffer(),
			0, 0, 
			pScaleBitmap_->GetWidth(), 
			pScaleBitmap_->GetHeight(),
			0, 0, 
			pSlot->glyphBitmap.GetWidth(),
			pSlot->glyphBitmap.GetHeight(),
			pSlot->glyphBitmap.GetWidth()
		);
	}
	else
	{
		if (!fontRenderer_.GetGlyph(&pSlot->glyphBitmap, &pSlot->charWidth, &pSlot->charHeight,
			pSlot->charOffsetX, pSlot->charOffsetY, 0, 0, cChar))
		{
			// failed to render
			return false;
		}
	}

	// Blur it baby!
	if (smoothMethod_ == FontSmooth::BLUR)
	{
		pSlot->glyphBitmap.Blur(smoothAmount_);
	}

	pSlot->usage = usage_;
	pSlot->currentChar = cChar;

	cacheTable_.insert(std::make_pair(cChar, pSlot));
	return true;
}

bool XGlyphCache::UnCacheGlyph(wchar_t cChar)
{
	XCacheTable::iterator pItor = cacheTable_.find(cChar);

	if (pItor != cacheTable_.end())
	{
		XCacheSlot* pSlot = pItor->second;
		pSlot->reset();
		cacheTable_.erase(pItor);
		return true;
	}

	return false;
}

bool XGlyphCache::GlyphCached(wchar_t cChar)
{
	return (cacheTable_.find(cChar) != cacheTable_.end());
}

//------------------------------------------------------------------------------------------------- 

XCacheSlot* XGlyphCache::GetLRUSlot(void)
{
	uint32_t		minUsage = 0xffffffff;
	XCacheSlot		*pLRUSlot = 0;
	XCacheSlot		*pSlot;

	XCacheSlotListItor pItor = slotList_.begin();

	while (pItor != slotList_.end())
	{
		pSlot = *pItor;
		if (pSlot->usage == 0)
		{
			return pSlot;
		}
		else
		{
			if (pSlot->usage < minUsage)
			{
				pLRUSlot = pSlot;
				minUsage = pSlot->usage;
			}
		}
		++pItor;
	}
	return pLRUSlot;
}

//------------------------------------------------------------------------------------------------- 

XCacheSlot* XGlyphCache::GetMRUSlot(void)
{
	uint32_t		maxUsage = 0;
	XCacheSlot		*pMRUSlot = 0;
	XCacheSlot		*pSlot;

	XCacheSlotListItor pItor = slotList_.begin();

	while (pItor != slotList_.end())
	{
		pSlot = *pItor;

		if (pSlot->usage != 0)
		{
			if (pSlot->usage > maxUsage)
			{
				pMRUSlot = pSlot;
				maxUsage = pSlot->usage;
			}
		}
		++pItor;
	}
	return pMRUSlot;
}

//------------------------------------------------------------------------------------------------- 

bool XGlyphCache::GetGlyph(XGlyphBitmap** pGlyph, int32_t* pWidth, int32_t* pHeight, 
	int8_t& charOffsetX, int8_t& charOffsetY, wchar_t cChar)
{
	XCacheTable::iterator pItor = cacheTable_.find(cChar);

	// glyph already chached?
	if (pItor == cacheTable_.end())
	{
		if (!PreCacheGlyph(cChar))
		{
			return false;
		}
	}

	// should be in the cache table now.
	pItor = cacheTable_.find(cChar);

	X_ASSERT_NOT_NULL(pItor->second);

	pItor->second->usage = usage_++;
	(*pGlyph) = &pItor->second->glyphBitmap;

	if (pWidth) {
		*pWidth = pItor->second->charWidth;
	}

	if (pHeight) {
		*pHeight = pItor->second->charHeight;
	}

	charOffsetX = pItor->second->charOffsetX;
	charOffsetY = pItor->second->charOffsetY;

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

		if (!pCacheSlot->glyphBitmap.Create(glyphBitmapWidth_, glyphBitmapHeight_))
		{
			X_DELETE(pCacheSlot, arena_);
			return false;
		}

		pCacheSlot->reset();
		pCacheSlot->cacheSlot = static_cast<uint32_t>(i);

		slotList_.push_back(pCacheSlot);
	}

	return true;
}

//------------------------------------------------------------------------------------------------- 

void XGlyphCache::ReleaseSlotList(void)
{
	XCacheSlotListItor pItor = slotList_.begin();

	while (pItor != slotList_.end())
	{
		XCacheSlot* pSlot = (*pItor);
		++pItor;

		X_ASSERT_NOT_NULL(pSlot);
		X_DELETE(pSlot, arena_);
	}

	slotList_.free();
}



X_NAMESPACE_END
