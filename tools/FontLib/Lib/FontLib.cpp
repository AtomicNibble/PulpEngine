#include "stdafx.h"
#include "FontLib.h"

#include <IFileSys.h>

X_NAMESPACE_BEGIN(font)

	FontLib::FontLib()
	{
	}

	FontLib::~FontLib()
	{

	}

	const char* FontLib::getOutExtension(void) const
	{
		return FONT_FILE_EXTENSION;
	}

	bool FontLib::Convert(IConverterHost& host, int32_t assetId, ConvertArgs& args, const OutPath& destPath)
	{
		X_UNUSED(host, assetId, args, destPath);

	

		return true;
	}


X_NAMESPACE_END