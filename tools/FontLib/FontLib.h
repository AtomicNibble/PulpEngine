#pragma once

#include <Core\Compiler.h>

#ifndef FONTLIB_EXPORT
#ifdef X_LIB
#define FONTLIB_EXPORT
#else
#ifdef FONT_LIB_EXPORT
#define FONTLIB_EXPORT X_EXPORT
#else
#define FONTLIB_EXPORT X_IMPORT
#endif // !FONT_LIB_EXPORT
#endif // X_LIB
#endif // !FONTLIB_EXPORT

#include "FontRender\XFontGlyph.h"
#include "FontRender\XGlyphBitmap.h"
#include "FontRender\XFontRender.h"