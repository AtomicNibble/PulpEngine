#include "stdafx.h"
#include "TextureVars.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(texture)


	TextureVars::TextureVars()
	{
		// defaults
		allowRawImgLoading_ = 1;
	}

	void TextureVars::RegisterVars(void)
	{
		ADD_CVAR_REF("img_allowRawLoading", allowRawImgLoading_, allowRawImgLoading_, 0, 1,
			core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
			"Allows loading of none compiled images. dds,tga,png,jpg,psd...");
	}

X_NAMESPACE_END