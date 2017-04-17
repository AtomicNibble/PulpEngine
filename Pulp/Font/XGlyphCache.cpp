#include "stdafx.h"
#include "XGlyphCache.h"
#include "XFontTexture.h"

#include "Vars\FontVars.h"

#include <IFileSys.h>
#include <Threading\JobSystem2.h>

X_NAMESPACE_BEGIN(font)

namespace
{
	struct JobData
	{
		uint8_t* pData;
		uint32_t dataSize;
	};

} // naemspace

XGlyphCache::XGlyphCache(const FontVars& vars, core::MemoryArenaBase* arena) :
	vars_(vars),
	glyphBitmapWidth_(0),
	glyphBitmapHeight_(0),
	scaledGlyphWidth_(0),
	scaledGlyphHeight_(0),

	scaleBitmap_(arena),

	usage_(0),

	smoothMethod_(FontSmooth::NONE),
	smoothAmount_(FontSmoothAmount::NONE),

	slotList_(arena),
	cacheTable_(arena, 8),

	loadStatus_(LoadStatus::NotLoaded)	
{

}

XGlyphCache::~XGlyphCache()
{

}

void XGlyphCache::IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
	core::XFileAsync* pFile, uint32_t bytesTransferred)
{
	X_UNUSED(fileSys);
	X_UNUSED(bytesTransferred);

	X_ASSERT(pRequest->getType() == core::IoRequest::OPEN_READ_ALL, "Recived unexpected request type")(pRequest->getType());
	const core::IoRequestOpenRead* pOpenRead = static_cast<const core::IoRequestOpenRead*>(pRequest);

	if (!pFile) {
		loadStatus_ = LoadStatus::Error;
		X_ERROR("Font", "Error reading font data");
		return;
	}

	loadStatus_ = LoadStatus::Processing;

	JobData data;
	data.pData = static_cast<uint8_t*>(pOpenRead->pBuf);
	data.dataSize = pOpenRead->dataSize;

	// dispatch a job to parse it?
	gEnv->pJobSys->CreateMemberJobAndRun<XGlyphCache>(this, &XGlyphCache::ProcessFontFile_job, data);
}


void XGlyphCache::ProcessFontFile_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_UNUSED(jobSys);
	X_UNUSED(threadIdx);
	X_UNUSED(pJob);

	// we have the font data now we just need to setup the font render.
	JobData* pJobData = static_cast<JobData*>(pData);

	if (!fontRenderer_.SetRawFontBuffer(core::UniquePointer<uint8_t[]>(g_fontArena, pJobData->pData), 
		pJobData->dataSize, 
		FontEncoding::Unicode))
	{
		loadStatus_ = LoadStatus::Error;
		X_ERROR("Font", "Error setting up font renderer");
		return;
	}

	if(scaledGlyphWidth_) {
		fontRenderer_.SetGlyphBitmapSize(scaledGlyphWidth_, scaledGlyphHeight_);
	}
	else {
		fontRenderer_.SetGlyphBitmapSize(glyphBitmapWidth_, glyphBitmapHeight_);
	}

	// preform the precache in this job.
	if (vars_.glyphCachePreWarm()) {
		PreWarmCache();
	}

	// now we are ready.
	loadStatus_ = LoadStatus::Complete;
}


bool XGlyphCache::LoadGlyphSource(const SourceNameStr& name, bool async)
{
	// are we loading already?
	if (loadStatus_ == LoadStatus::Loading || loadStatus_ == LoadStatus::Processing) {
		return true;
	}

	core::Path<char> path;
	path /= "Fonts/";
	path.setFileName(name.begin(), name.end());

	core::fileModeFlags mode;
	mode.Set(core::fileMode::READ);
	mode.Set(core::fileMode::SHARE);

	if (async)
	{
		loadStatus_ = LoadStatus::Loading;

		// load the file async
		core::IoRequestOpenRead open;
		open.callback.Bind<XGlyphCache, &XGlyphCache::IoRequestCallback>(this);
		open.mode = mode;
		open.path = path;
		open.arena = g_fontArena;

		gEnv->pFileSys->AddIoRequestToQue(open);
	}
	else
	{
		core::XFileScoped file;

		if (!file.openFile(path.c_str(), core::fileModeFlags::READ)) {
			return false;
		}

		const size_t fileSize = safe_static_cast<size_t>(file.remainingBytes());
		if (!fileSize) {
			X_ERROR("Font", "Font file is zero bytes in size");
			return false;
		}

		core::UniquePointer<uint8_t[]> buf = core::makeUnique<uint8_t[]>(g_fontArena, fileSize);
		if (file.read(buf.ptr(), fileSize) != fileSize) {
			X_ERROR("Font", "Error reading font data");
			return false;
		}

		if (!fontRenderer_.SetRawFontBuffer(std::move(buf), safe_static_cast<int32_t>(fileSize), FontEncoding::Unicode)) {
			X_ERROR("Font", "Error setting up font renderer");
			return false;
		}

		if (scaledGlyphWidth_) {
			fontRenderer_.SetGlyphBitmapSize(scaledGlyphWidth_, scaledGlyphHeight_);
		}
		else {
			fontRenderer_.SetGlyphBitmapSize(glyphBitmapWidth_, glyphBitmapHeight_);
		}

		if (vars_.glyphCachePreWarm()) {
			PreWarmCache();
		}
	}

	return true;
}

bool XGlyphCache::Create(int32_t glyphBitmapWidth, int32_t glyphBitmapHeight)
{
	int32_t cacheSize = vars_.glyphCacheSize();

	smoothMethod_ = vars_.fontSmoothMethod();
	smoothAmount_ = vars_.fontSmoothAmount();

	glyphBitmapWidth_ = glyphBitmapWidth;
	glyphBitmapHeight_ = glyphBitmapHeight;
	scaledGlyphWidth_ = 0;
	scaledGlyphHeight_ = 0;

	if (!CreateSlotList(cacheSize)) {
		ReleaseSlotList();
		return false;
	}

	switch (smoothAmount_)
	{
		case FontSmooth::SUPERSAMPLE:
		{
			switch (smoothAmount_)
			{
				case FontSmoothAmount::X2:
					scaledGlyphWidth_ = glyphBitmapWidth_ << 1;
					scaledGlyphHeight_ = glyphBitmapHeight_ << 1;
				break;
				case FontSmoothAmount::X4:
					scaledGlyphWidth_ = glyphBitmapWidth_ << 2;
					scaledGlyphHeight_ = glyphBitmapHeight_ << 2;
				break;
			}
		}
		break;
		default:
			break;
	}

	// Scaled?
	if (scaledGlyphWidth_ > 0)
	{
		scaleBitmap_.reset(X_NEW(XGlyphBitmap, scaleBitmap_.getArena(), "BitMap"));

		if (!scaleBitmap_->Create(scaledGlyphWidth_, scaledGlyphHeight_))
		{
			Release();
			return false;
		}
	}


	return true;
}

void XGlyphCache::Release(void)
{
	fontRenderer_.Release();

	ReleaseSlotList();

	cacheTable_.clear();

	scaleBitmap_.release();

	glyphBitmapWidth_ = 0;
	glyphBitmapHeight_ = 0;
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

void XGlyphCache::PreWarmCache(void)
{
	X_ASSERT(cacheTable_.empty(), "Can only be run when the cache is empty")(cacheTable_.size());

	wchar_t buf[256];
	wchar_t* p = buf;

	wchar_t i = L' ';

	for (; i <= L'~'; ++i) {
		*p++ = i;
	}

	i += 35;

	for (; i < 256; ++i) {
		*p++ = i;
	}

	size_t len = (p - buf);
	len = core::Min(len, slotList_.size()); // only precache what we can fit in the cache.

	X_ASSERT(len > 0, "Cache must not be zero in size")(slotList_.size());

	++usage_; // give them fake usage.

	for (size_t x = 0; x < len; x++) {
		PreCacheGlyph(buf[x]);
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
		int32_t iterations = 1;

		switch (smoothAmount_)
		{
			case FontSmoothAmount::X2:
				iterations = 2;
				break;
			case FontSmoothAmount::X4:
				iterations = 4;
				break;
		}

		pSlot->glyphBitmap.Blur(iterations);
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
