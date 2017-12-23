#include "stdafx.h"
#include "TextureVars.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(engine)


TextureVars::TextureVars()
{
	// defaults
	allowRawImgLoading_ = 1;

	allowFmtDDS_ = 1;
	allowFmtPNG_ = 1;
	allowFmtJPG_ = 1;
	allowFmtPSD_ = 1;
	allowFmtTGA_ = 1;
}

void TextureVars::registerVars(void)
{
	ADD_CVAR_REF("img_allow_raw_fmt", allowRawImgLoading_, allowRawImgLoading_, 0, 1,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Allows loading of none compiled images. dds,tga,png,jpg,psd...");


	ADD_CVAR_REF("img_fmt_dds", allowFmtDDS_, allowFmtDDS_, 0, 1,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED | core::VarFlag::RESTART_REQUIRED,
		"Enable loading of dds textures");
	ADD_CVAR_REF("img_fmt_png", allowFmtPNG_, allowFmtPNG_, 0, 1,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED | core::VarFlag::RESTART_REQUIRED,
		"Enable loading of png textures");
	ADD_CVAR_REF("img_fmt_jpg", allowFmtJPG_, allowFmtJPG_, 0, 1,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED | core::VarFlag::RESTART_REQUIRED,
		"Enable loading of jpg textures");
	ADD_CVAR_REF("img_fmt_psd", allowFmtPSD_, allowFmtPSD_, 0, 1,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED | core::VarFlag::RESTART_REQUIRED,
		"Enable loading of psd textures");
	ADD_CVAR_REF("img_fmt_tga", allowFmtTGA_, allowFmtTGA_, 0, 1,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED | core::VarFlag::RESTART_REQUIRED,
		"Enable loading of tga textures");
}


X_NAMESPACE_END