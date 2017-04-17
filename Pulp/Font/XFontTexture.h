#pragma once

#ifndef _X_FONT_TEXTURE_H_
#define _X_FONT_TEXTURE_H_

#include "XGlyphCache.h"

X_NAMESPACE_BEGIN(font)

// the glyph spacing in font texels between characters in proportional font mode (more correct would be to take the value in the character)
// #define X_FONT_GLYPH_PROP_SPACING		(1)
// the size of a rendered space, this value gets multiplied by the default characted width
// #define X_FONT_SPACE_SIZE				(0.5f)
// don't draw this char (used to avoid drawing color codes)
// #define X_FONT_NOT_DRAWABLE_CHAR		(0xffff)

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
	int32_t		textureSlot;		
	float		texCoord[2];		// character position in the texture (not yet half texel corrected)
	uint8_t		charWidth;			// size in pixel
	uint8_t		charHeight;			// size in pixel
	int8_t		charOffsetX;
	int8_t		charOffsetY;

	X_INLINE void reset(void)
	{
		slotUsage = 0;
		currentChar = static_cast<wchar_t>(~0);
		charWidth = 0;
		charHeight = 0;
		charOffsetX = 0;
		charOffsetY = 0;
	}

	X_INLINE void setNotReusable(void) { // this slot can't be reused for somthing else.
		slotUsage = 0xffff;
	}

};

// keep this small
X_ENSURE_SIZE(XTextureSlot, 20);

struct XCharCords
{
	Vec4<float> texCoords;
	Vec2<int> size;
	Vec2<int> offset;
};


class XFontTexture
{
	typedef core::Array<XTextureSlot>					XTextureSlotList;
	typedef XTextureSlotList::Iterator					XTextureSlotListItor;
	typedef core::Array<uint8_t>						BufferArr;

	typedef core::HashMap<uint16, XTextureSlot*>		XTextureSlotTable;
	typedef XTextureSlotTable::iterator					XTextureSlotTableItor;
	typedef XTextureSlotTable::const_iterator			XTextureSlotTableItorConst;

	static const uint32_t FONT_GLYPH_CACHE_SIZE = 256; // todo make var?

public:
	XFontTexture(core::MemoryArenaBase* arena);
	~XFontTexture();

	void ClearBuffer(void);

	bool CreateFromMemory(BufferArr& buf, int32_t width, int32_t height, 
		FontSmooth::Enum  smoothMethod, FontSmoothAmount::Enum smoothAmount, float sizeRatio = 0.875f, 
		int32_t widthCharCount = 16, int32_t heightCharCount = 16);


	// returns 1 if texture updated, returns 2 if texture not updated, returns 0 on error
	// pUpdated is the number of slots updated
	CacheResult::Enum PreCacheString(const wchar_t* pBegin, const wchar_t* pEnd, int32_t* pUpdatedOut = nullptr);

	int32_t GetCharacterWidth(wchar_t cChar) const;
	void GetTextureCoord(const XTextureSlot* pSlot, XCharCords& cords) const;


	X_INLINE const Vec2i GetSize(void) const;
	X_INLINE const int32_t GetWidth(void) const;
	X_INLINE const int32_t GetHeight(void) const;
	X_INLINE const BufferArr& GetBuffer(void) const;

	X_INLINE float GetTextureCellWidth(void) const;
	X_INLINE float GetTextureCellHeight(void) const;

	X_INLINE int32_t WidthCellCellCount(void) const;
	X_INLINE int32_t HeightCellCount(void) const;

	X_INLINE int32_t GetSlotUsage(void);

	// useful for special feature rendering interleaved with fonts (e.g. box behind the text)
	void CreateGradientSlot(void);

	wchar_t GetSlotChar(int32_t slot) const;
	XTextureSlot* GetCharSlot(wchar_t cChar);
	XTextureSlot* GetGradientSlot(void);
	XTextureSlot* GetLRUSlot(void);
	XTextureSlot* GetMRUSlot(void);

public:
	// writes the texture to a file.
	bool WriteToFile(const char* filename);

private:
	bool Create(int32_t width, int32_t height, FontSmooth::Enum smoothMethod, FontSmoothAmount::Enum smoothAmount,
		float sizeRatio = 0.8f, int32_t widthCharCount = 16, int32_t heightCharCount = 16);

private:
	bool CreateSlotList(int32_t listSize);
	bool ReleaseSlotList(void);
	bool UpdateSlot(XTextureSlot* pSlot, uint16 slotUsage, wchar_t cChar);

private:
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

	int32_t	textureSlotCount_;

	FontSmooth::Enum		smoothMethod_;
	FontSmoothAmount::Enum	smoothAmount_;

	XGlyphCache			glyphCache_;
	XTextureSlotList	slotList_;
	XTextureSlotTable	slotTable_;

	uint16	slotUsage_;
};


X_NAMESPACE_END

#include "XFontTexture.inl"

#endif // !_X_FONT_TEXTURE_H_
