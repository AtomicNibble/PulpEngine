#pragma once

#include "XGlyphBitmap.h"


X_NAMESPACE_BEGIN(font)



struct XGlyph
{
	XGlyph() :
		glyphBitmap(g_fontArena)
	{

	}

	X_INLINE void reset(void)
	{
		usage = 0;
		cacheSlot = 0;
		currentChar = static_cast<wchar_t>(~0);

		advanceX = 0;
		charWidth = 0;
		charHeight = 0;
		charOffsetX = 0;
		charOffsetY = 0;

		glyphBitmap.Clear();
	}

public:
	uint32			usage;
	int32_t			cacheSlot;
	wchar_t			currentChar;

	uint16_t		advanceX;
	uint8_t			charWidth;		// size in pixel
	uint8_t			charHeight;		// size in pixel
	int8_t			charOffsetX;	// these can be negative.
	int8_t			charOffsetY;

	XGlyphBitmap	glyphBitmap;
};

X_ENSURE_SIZE(XGlyph, 64);

X_NAMESPACE_END