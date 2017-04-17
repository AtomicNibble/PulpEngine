#include "stdafx.h"
#include "FontVars.h"

#include <IConsole.h>


X_NAMESPACE_BEGIN(font)

FontVars::FontVars()
{
	glyphCacheSize_ = 256;
	glyphCachePreWarm_ = 1;

}


void FontVars::registerVars(void)
{

	ADD_CVAR_REF("font_glyph_cache_size", glyphCacheSize_, glyphCacheSize_, 1, 1 << 14,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED | core::VarFlag::RESTART_REQUIRED,
		"Specifies the per font file glyph cache size");

	ADD_CVAR_REF("font_glyph_cache_prewarm", glyphCachePreWarm_, glyphCachePreWarm_, 0, 1,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Warm the glyph cache with common ascii chars");


}

X_NAMESPACE_END