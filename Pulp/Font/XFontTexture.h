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
	char		iCharOffsetX;
	char		iCharOffsetY;

	void Reset(void)
	{
		wSlotUsage = 0;
		cCurrentChar = static_cast<wchar_t>(~0);
		iCharWidth = 0;
		iCharHeight = 0;
		iCharOffsetX = 0;
		iCharOffsetY = 0;
	}

	void SetNotReusable(void) { // this slot can't be reused for somthing else.
		wSlotUsage = 0xffff;
	}

};


typedef std::vector<XTextureSlot *>								XTextureSlotList;
typedef std::vector<XTextureSlot *>::iterator					XTextureSlotListItor;

typedef std::hash_map<uint16, XTextureSlot *>					XTextureSlotTable;
typedef std::hash_map<uint16, XTextureSlot *>::iterator			XTextureSlotTableItor;
typedef std::hash_map<uint16, XTextureSlot *>::const_iterator	XTextureSlotTableItorConst;

struct XCharCords
{
	Vec4<float >texCoords;
	Vec2<int> size;
	Vec2<int> offset;
};


class XFontTexture
{
public:
	XFontTexture();
	~XFontTexture();

	int Release();

	bool CreateFromMemory(BYTE *pFileData, size_t iDataSize, int iWidth,
			int iHeight, int iSmoothMethod, int iSmoothAmount, 
			float fSizeRatio = 0.875f, int iWidthCharCount = 16, 
			int iHeightCharCount = 16);

	bool Create(int iWidth, int iHeight, int iSmoothMethod, int iSmoothAmount,
		float fSizeRatio = 0.8f, int iWidthCharCount = 16, int iHeightCharCount = 16);

	// returns 1 if texture updated, returns 2 if texture not updated, returns 0 on error
	// pUpdated is the number of slots updated
	int PreCacheString(const wchar_t* szString, int* pUpdated = 0);

	int GetCharacterWidth(wchar_t cChar) const;
	void GetTextureCoord(XTextureSlot* pSlot, XCharCords& cords) const;


	X_INLINE const Vec2i GetSize() const { return Vec2i(width_, height_); }
	X_INLINE const int GetWidth() const { return width_; }
	X_INLINE const int GetHeight() const{ return height_; }
	X_INLINE uint8* GetBuffer() { return pBuffer_; }

	X_INLINE float GetTextureCellWidth(void) const { return fTextureCellWidth_; }
	X_INLINE float GetTextureCellHeight(void) const { return fTextureCellHeight_; }

	X_INLINE int WidthCellCellCount(void) const { return iWidthCellCount_; }
	X_INLINE int HeightCellCount(void) const { return iHeightCellCount_; }

	X_INLINE int GetSlotUsage(void) { return wSlotUsage_; }

	// useful for special feature rendering interleaved with fonts (e.g. box behind the text)
	void CreateGradientSlot();

	wchar_t GetSlotChar(int slot) const;
	XTextureSlot* GetCharSlot(wchar_t cChar);
	XTextureSlot* GetGradientSlot();
	XTextureSlot* GetLRUSlot();
	XTextureSlot* GetMRUSlot();

public:
	// writes the texture to a file.
	bool WriteToFile(const char* filename);

private:
	int CreateSlotList(int iListSize);
	int ReleaseSlotList();
	int UpdateSlot(int iSlot, uint16 wSlotUsage, wchar_t cChar);

private:
	int width_;
	int height_;

	uint8* pBuffer_;

	float		fInvWidth_;
	float		fInvHeight_;

	int			iCellWidth_;
	int			iCellHeight_;

	float		fTextureCellWidth_;
	float		fTextureCellHeight_;

	int			iWidthCellCount_;
	int			iHeightCellCount_;

	int			nTextureSlotCount_;

	int			iSmoothMethod_;
	int			iSmoothAmount_;

	XGlyphCache			GlyphCache_;
	XTextureSlotList	pSlotList_;
	XTextureSlotTable	pSlotTable_;

	uint16				wSlotUsage_;
};


X_NAMESPACE_END

#endif // !_X_FONT_TEXTURE_H_
