#include "stdafx.h"
#include "FontVars.h"

#include <IConsole.h>


X_NAMESPACE_BEGIN(font)

FontVars::FontVars()
{
	glyphCacheSize_ = 512;
	glyphCachePreWarm_ = 1;

	fontSmoothingMethod_ = 0;
	fontSmoothingAmount_ = 0;
}


void FontVars::registerVars(void)
{

	ADD_CVAR_REF("font_glyph_cache_size", glyphCacheSize_, glyphCacheSize_, 1, 1 << 14,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED | core::VarFlag::RESTART_REQUIRED,
		"Specifies the per font file glyph cache size");

	ADD_CVAR_REF("font_glyph_cache_prewarm", glyphCachePreWarm_, glyphCachePreWarm_, 0, 1,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Warm the glyph cache with common ascii chars");

	ADD_CVAR_REF("font_glyph_smoothing", fontSmoothingMethod_, FontSmooth::NONE, 0, FontSmooth::ENUM_COUNT,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED | core::VarFlag::RESTART_REQUIRED,
		"Type of smoothing to apply to rendered glyphs. 0. none 1. blur 2. supersample");

	ADD_CVAR_REF("font_glyph_smoothing_amount", fontSmoothingAmount_, FontSmoothAmount::NONE, 0, FontSmoothAmount::ENUM_COUNT,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED | core::VarFlag::RESTART_REQUIRED,
		"How much smoothing to apply for blue/supersample. 0. none 1. x2 3. x4");

}


X_NAMESPACE_END