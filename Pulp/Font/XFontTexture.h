#pragma once

#ifndef _X_FONT_TEXTURE_H_
#define _X_FONT_TEXTURE_H_

#include "XGlyphCache.h"

X_NAMESPACE_BEGIN(font)

#define	X_FONT_GLYPH_CACHE_SIZE		(1)
// the glyph spacing in font texels between characters in proportional font mode (more correct would be to take the value in the character)
// #define X_FONT_GLYPH_PROP_SPACING		(1)
// the size of a rendered space, this value gets multiplied by the default characted width
// #define X_FONT_SPACE_SIZE				(0.5f)
// don't draw this char (used to avoid drawing color codes)
// #define X_FONT_NOT_DRAWABLE_CHAR		(0xffff)

// we cache glyphs with "Least Recently Used (LRU)"
// so if we run out of slots the slot that is LRU
// is replaced.

struct FontSmooth
{
	enum Enum
	{
		NONE,
		BLUR,
		SUPERSAMPLE
	};
};

struct FontSmoothAmount
{
	enum Enum
	{
		NONE,
		X2,
		X4
	};
};



struct XTextureSlot
{
	uint16		wSlotUsage;			// for LRU strategy, 0xffff is never released
	wchar_t		cCurrentChar;		// ~0 if not used for characters
	int			iTextureSlot;		
	float		vTexCoord[2];		// character position in the texture (not yet half texel corrected)
	uint8		iCharWidth;			// size in pixel
	uint8		iCharHeight;		// size in pixel
	int8_t		iCharOffsetX;
	int8_t		iCharOffsetY;

	void reset(void)
	{
		wSlotUsage = 0;
		cCurrentChar = static_cast<wchar_t>(~0);
		iCharWidth = 0;
		iCharHeight = 0;
		iCharOffsetX = 0;
		iCharOffsetY = 0;
	}

	void setNotReusable(void) { // this slot can't be reused for somthing else.
		wSlotUsage = 0xffff;
	}

};


struct XCharCords
{
	Vec4<float >texCoords;
	Vec2<int> size;
	Vec2<int> offset;
};


class XFontTexture
{
	typedef core::Array<XTextureSlot *>					XTextureSlotList;
	typedef core::Array<XTextureSlot *>::Iterator		XTextureSlotListItor;

	typedef core::HashMap<uint16, XTextureSlot *>		XTextureSlotTable;
	typedef XTextureSlotTable::iterator					XTextureSlotTableItor;
	typedef XTextureSlotTable::const_iterator			XTextureSlotTableItorConst;


public:
	XFontTexture(core::MemoryArenaBase* arena);
	~XFontTexture();

	int32_t Release(void);

	bool CreateFromMemory(uint8_t* pFileData, size_t iDataSize, int iWidth,
			int32_t iHeight, int32_t iSmoothMethod, int32_t iSmoothAmount,
			float fSizeRatio = 0.875f, int32_t iWidthCharCount = 16,
		int32_t iHeightCharCount = 16);

	bool Create(int32_t iWidth, int32_t iHeight, int32_t iSmoothMethod, int32_t iSmoothAmount,
		float fSizeRatio = 0.8f, int32_t iWidthCharCount = 16, int32_t iHeightCharCount = 16);

	// returns 1 if texture updated, returns 2 if texture not updated, returns 0 on error
	// pUpdated is the number of slots updated
	int32_t PreCacheString(const wchar_t* szString, int32_t* pUpdated = 0);

	int32_t GetCharacterWidth(wchar_t cChar) const;
	void GetTextureCoord(XTextureSlot* pSlot, XCharCords& cords) const;


	X_INLINE const Vec2i GetSize(void) const;
	X_INLINE const int GetWidth(void) const;
	X_INLINE const int GetHeight(void) const;
	X_INLINE uint8* GetBuffer(void);

	X_INLINE float GetTextureCellWidth(void) const;
	X_INLINE float GetTextureCellHeight(void) const;

	X_INLINE int32_t WidthCellCellCount(void) const;
	X_INLINE int32_t HeightCellCount(void) const;

	X_INLINE int32_t GetSlotUsage(void);

	// useful for special feature rendering interleaved with fonts (e.g. box behind the text)
	void CreateGradientSlot(void);

	wchar_t GetSlotChar(int slot) const;
	XTextureSlot* GetCharSlot(wchar_t cChar);
	XTextureSlot* GetGradientSlot(void);
	XTextureSlot* GetLRUSlot(void);
	XTextureSlot* GetMRUSlot(void);

public:
	// writes the texture to a file.
	bool WriteToFile(const char* filename);

private:
	int32_t CreateSlotList(int iListSize);
	int32_t ReleaseSlotList();
	int32_t UpdateSlot(int iSlot, uint16 wSlotUsage, wchar_t cChar);

private:
	int32_t width_;
	int32_t height_;

	uint8*	pBuffer_;

	float		fInvWidth_;
	float		fInvHeight_;

	int32_t		iCellWidth_;
	int32_t		iCellHeight_;

	float		fTextureCellWidth_;
	float		fTextureCellHeight_;

	int32_t		iWidthCellCount_;
	int32_t		iHeightCellCount_;

	int32_t		nTextureSlotCount_;

	int32_t		iSmoothMethod_;
	int32_t		iSmoothAmount_;

	XGlyphCache			glyphCache_;
	XTextureSlotList	slotList_;
	XTextureSlotTable	slotTable_;

	uint16				wSlotUsage_;
};


X_NAMESPACE_END

#include "XFontTexture.inl"

#endif // !_X_FONT_TEXTURE_H_
