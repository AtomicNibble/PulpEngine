#pragma once

#ifndef _X_FONT_TEXTURE_H_
#define _X_FONT_TEXTURE_H_

#include "XGlyphCache.h"


X_NAMESPACE_DECLARE(core,
	struct IoRequestBase;
	struct XFileAsync;
);


X_NAMESPACE_BEGIN(font)


class FontVars;

// we cache glyphs with "Least Recently Used (LRU)"
// so if we run out of slots the slot that is LRU
// is replaced.


X_DECLARE_ENUM(CacheResult)(
	UPDATED,
	UNCHANGED,
	ERROR
);

struct XTextureSlot
{
	uint16_t	slotUsage;			// for LRU strategy, 0xffff is never released
	wchar_t		currentChar;		// ~0 if not used for characters
	float		texCoord[2];		// character position in the texture (not yet half texel corrected)
	uint8_t		charWidth;			// size in pixel
	uint8_t		charHeight;			// size in pixel
	int8_t		charOffsetX;
	int8_t		charOffsetY;
	int8_t		paddingX;
	int8_t		paddingY;
	uint16_t	advanceX;
//	uint8_t		advanceY;
	int32_t		textureSlot;		

	X_INLINE void reset(void)
	{
		slotUsage = 0;
		currentChar = static_cast<wchar_t>(~0);
		charWidth = 0;
		charHeight = 0;
		charOffsetX = 0;
		charOffsetY = 0;
		paddingX = 0;
		paddingY = 0;
		advanceX = 0;
	//	advanceY = 0;
	}

	X_INLINE void setNotReusable(void) { // this slot can't be reused for somthing else.
		slotUsage = 0xffff;
	}

	X_INLINE int32_t getCharacterWidth(void) const {
		return charWidth + 1;
	}
};

// keep this small
X_ENSURE_SIZE(XTextureSlot, 24);

struct XCharCords
{
	Vec4<float> texCoords;
	Vec2<int> size;
	Vec2<int> offset;
};

// This FontTexture creates a texter of width x height and then creates a LRU slot cache across the buffer 
// initially the cache is completly empty.
// 
// The cache is then updated via calls to PreCacheString, which then copyies rendered Glyph's into our 
// cpu texture buffer and updates the LRU slot info.
// 
class XFontTexture : public core::ReferenceCounted<>
{
	typedef core::Array<XTextureSlot>					XTextureSlotList;
	typedef XTextureSlotList::Iterator					XTextureSlotListItor;
	typedef core::Array<uint8_t>						BufferArr;

	typedef core::HashMap<uint16, XTextureSlot*>		XTextureSlotTable;
	typedef XTextureSlotTable::iterator					XTextureSlotTableItor;
	typedef XTextureSlotTable::const_iterator			XTextureSlotTableItorConst;

public:
	XFontTexture(const SourceNameStr& name, const FontVars& vars, core::MemoryArenaBase* arena);
	~XFontTexture();

	bool Create(int32_t width, int32_t height, int32_t widthCharCount, int32_t heightCharCount);
	
	X_INLINE bool IsReady(void) const;
	bool WaitTillReady(void);
	bool LoadGlyphSource(bool async);

	// returns 1 if texture updated, returns 2 if texture not updated, returns 0 on error
	// pUpdated is the number of slots updated
	CacheResult::Enum PreCacheString(const wchar_t* pBegin, const wchar_t* pEnd, int32_t* pUpdatedOut = nullptr);

	void GetTextureCoord(const XTextureSlot* pSlot, XCharCords& cords) const;

	X_INLINE const SourceNameStr& GetName(void) const;
	X_INLINE const Vec2i GetSize(void) const;
	X_INLINE const int32_t GetWidth(void) const;
	X_INLINE const int32_t GetHeight(void) const;
	X_INLINE const BufferArr& GetBuffer(void) const;

	X_INLINE int32_t GetCellWidth(void) const;
	X_INLINE int32_t GetCellHeight(void) const;

	X_INLINE float GetTextureCellWidth(void) const;
	X_INLINE float GetTextureCellHeight(void) const;

	X_INLINE int32_t WidthCellCellCount(void) const;
	X_INLINE int32_t HeightCellCount(void) const;

	X_INLINE int32_t GetSlotUsage(void);
	X_INLINE int32_t GetCacheMisses(void);

	X_INLINE const Metrics& GetMetrics(void) const;

	wchar_t GetSlotChar(int32_t slot) const;
	XTextureSlot* GetCharSlot(wchar_t cChar);
	XTextureSlot* GetGradientSlot(void);
	XTextureSlot* GetLRUSlot(void);
	XTextureSlot* GetMRUSlot(void);

public:
	// writes the texture to a file.
	bool WriteToFile(const char* filename);

private:
	void Clear(void); 

	bool CreateSlotList(int32_t listSize);
	bool ReleaseSlotList(void);
	bool UpdateSlot(XTextureSlot* pSlot, uint16 slotUsage, wchar_t cChar);
	
	// useful for special feature rendering interleaved with fonts (e.g. box behind the text)
	void CreateGradientSlot(void);
	void PreWarmCache(void);

private:
	void IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
		core::XFileAsync* pFile, uint32_t bytesTransferred);

	void ProcessFontFile_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);

private:
	const FontVars& vars_;
	const SourceNameStr name_;
	XGlyphCache glyphCache_;
	core::MemoryArenaBase* textureSlotArea_;

	int32_t width_;
	int32_t height_;

	BufferArr textureBuffer_;

	float	invWidth_;
	float	invHeight_;

	int32_t	cellWidth_;
	int32_t	cellHeight_;

	float	textureCellWidth_;
	float	textureCellHeight_;

	int32_t	widthCellCount_;
	int32_t	heightCellCount_;

	XTextureSlotList	slotList_;
	XTextureSlotTable	slotTable_;

	uint16	slotUsage_;
	uint32_t cacheMisses_;

	core::Signal signal_;
	core::LoadStatus::Enum loadStatus_;
};


X_NAMESPACE_END

#include "XFontTexture.inl"

#endif // !_X_FONT_TEXTURE_H_
