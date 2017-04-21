#include "stdafx.h"
#include "FontVars.h"

#include <IConsole.h>


X_NAMESPACE_BEGIN(font)

FontVars::FontVars()
{
	glyphCacheSize_ = 512;
	glyphCachePreWarm_ = 1;
	glyphDebugRender_ = 1;

	glyphDebugRect_ = 0;

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

	ADD_CVAR_REF("font_glyph_debug_render", glyphDebugRender_, glyphDebugRender_, 0, 1,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED | core::VarFlag::RESTART_REQUIRED,
		"Alter the glyphs to have color offset so the shape of the glyph is visible");

	ADD_CVAR_REF("font_glyph_debug_rect", glyphDebugRect_, glyphDebugRect_, 0, 1,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED | core::VarFlag::RESTART_REQUIRED,
		"Draw a bounding rect around each glyph");


	ADD_CVAR_REF("font_glyph_smoothing", fontSmoothingMethod_, FontSmooth::NONE, 0, FontSmooth::ENUM_COUNT,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED | core::VarFlag::RESTART_REQUIRED,
		"Type of smoothing to apply to rendered glyphs. 0. none 1. blur 2. supersample");

	ADD_CVAR_REF("font_glyph_smoothing_amount", fontSmoothingAmount_, FontSmoothAmount::NONE, 0, FontSmoothAmount::ENUM_COUNT,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED | core::VarFlag::RESTART_REQUIRED,
		"How much smoothing to apply for blue/supersample. 0. none 1. x2 2. x4");

}


X_NAMESPACE_END