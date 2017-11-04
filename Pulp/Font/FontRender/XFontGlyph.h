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
		currentChar = static_cast<wchar_t>(~0);

		advanceX = 0;
		charWidth = 0;
		charHeight = 0;
		charOffsetX = 0;
		charOffsetY = 0;
		bitmapOffsetX = 0;
		bitmapOffsetY = 0;

		glyphBitmap.Clear();
	}

public:
	uint32			usage;
	wchar_t			currentChar;

	uint16_t		advanceX;
	uint8_t			charWidth;		// size in pixel
	uint8_t			charHeight;		// size in pixel
	int8_t			charOffsetX;	// these can be negative.
	int8_t			charOffsetY;

	int8_t			bitmapOffsetX;
	int8_t			bitmapOffsetY;
	int8_t			_pad[2];

	XGlyphBitmap	glyphBitmap;
};

X_ENSURE_SIZE(XGlyph, 64);

X_NAMESPACE_END