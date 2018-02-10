#include "stdafx.h"
#include "FxLib.h"

#include <IFileSys.h>

#include "Compiler\Compiler.h"

X_NAMESPACE_BEGIN(engine)


namespace fx
{

	FxLib::FxLib()
	{
	}

	FxLib::~FxLib()
	{

	}

	const char* FxLib::getOutExtension(void) const
	{
		return EFFECT_FILE_EXTENSION;
	}

	bool FxLib::Convert(IConverterHost& host, int32_t assetId, ConvertArgs& args, const OutPath& destPath)
	{
		X_UNUSED(host, assetId, args, destPath);

		return true;
	}

} // namespace fx

X_NAMESPACE_END