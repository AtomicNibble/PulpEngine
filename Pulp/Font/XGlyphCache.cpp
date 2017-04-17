#include "stdafx.h"
#include "XGlyphCache.h"

#include "XFontTexture.h"

X_NAMESPACE_BEGIN(font)


XGlyphCache::XGlyphCache(core::MemoryArenaBase* arena) :
	arena_(arena),

	glyphBitmapWidth_(0),
	glyphBitmapHeight_(0),
	fSizeRatio_(0.8f),

	scaleBitmap_(arena),

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
	FontSmooth::Enum smoothMethod, FontSmoothAmount::Enum smoothAmount, float sizeRatio)
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
	if (scaledGlyphWidth > 0)
	{
		scaleBitmap_.reset(X_NEW(XGlyphBitmap, arena_,"BitMap"));

		if (!scaleBitmap_->Create(scaledGlyphWidth, scaledGlyphHeight))
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

	scaleBitmap_.release();

	glyphBitmapWidth_ = 0;
	glyphBitmapHeight_ = 0;
	fSizeRatio_ = 0.8f;
}

void XGlyphCache::ReleaseFont(void)
{
	fontRenderer_.Release();
}

bool XGlyphCache::LoadFontFromMemory(BufferArr& buf)
{
	return fontRenderer_.LoadFromMemory(buf);
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
	X_ASSERT(cacheTable_.find(cChar) == cacheTable_.end(), "Precache caleed when already in cache")();

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
	if (scaleBitmap_)
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

		scaleBitmap_->Clear();

		if (!fontRenderer_.GetGlyph(scaleBitmap_.ptr(), &pSlot->charWidth, &pSlot->charHeight,
				pSlot->charOffsetX, pSlot->charOffsetY, 0, 0, cChar))
		{
			// failed to render
			return false;
		}

		pSlot->charWidth >>= offsetMult >> 1;
		pSlot->charHeight >>= offsetMult >> 1;

		scaleBitmap_->BlitScaledTo8(
			pSlot->glyphBitmap.GetBuffer(),
			0, 0, 
			scaleBitmap_->GetWidth(),
			scaleBitmap_->GetHeight(),
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
	const auto it = std::min_element(slotList_.begin(), slotList_.end(), [](const XCacheSlot& s1, const XCacheSlot& s2) {
		return s1.usage < s2.usage;
	});

	auto& slot = *it;
	return &slot;
}

//------------------------------------------------------------------------------------------------- 

XCacheSlot* XGlyphCache::GetMRUSlot(void)
{
	const auto it = std::max_element(slotList_.begin(), slotList_.end(), [](const XCacheSlot& s1, const XCacheSlot& s2) {
		return s1.usage < s2.usage;
	});

	auto& slot = *it;
	return &slot;
}

//------------------------------------------------------------------------------------------------- 

bool XGlyphCache::GetGlyph(XGlyphBitmap*& pGlyphOut, int32_t* pWidth, int32_t* pHeight,
	int8_t& charOffsetX, int8_t& charOffsetY, wchar_t cChar)
{
	// glyph already chached?
	XCacheTable::iterator pItor = cacheTable_.find(cChar);
	if (pItor == cacheTable_.end())
	{
		if (!PreCacheGlyph(cChar))
		{
			X_ERROR("Font", "Failed to cache glyph for char: '%lc'", cChar);
			return false;
		}

		pItor = cacheTable_.find(cChar);
	}

	// should be in the cache table now.
	X_ASSERT_NOT_NULL(pItor->second);
	auto* pGlyph = pItor->second;

	pGlyph->usage = usage_++;

	pGlyphOut = &pGlyph->glyphBitmap;

	if (pWidth) {
		*pWidth = pGlyph->charWidth;
	}

	if (pHeight) {
		*pHeight = pGlyph->charHeight;
	}

	charOffsetX = pGlyph->charOffsetX;
	charOffsetY = pGlyph->charOffsetY;
	return true;
}

//------------------------------------------------------------------------------------------------- 

bool XGlyphCache::CreateSlotList(size_t listSize)
{
	slotList_.resize(listSize);

	for (size_t i = 0; i < listSize; i++)
	{
		XCacheSlot& slot = slotList_[i];
		if (!slot.glyphBitmap.Create(glyphBitmapWidth_, glyphBitmapHeight_))
		{
			return false;
		}

		slot.reset();
		slot.cacheSlot = static_cast<uint32_t>(i);
	}

	return true;
}

//------------------------------------------------------------------------------------------------- 

void XGlyphCache::ReleaseSlotList(void)
{
	slotList_.free();
}



X_NAMESPACE_END
