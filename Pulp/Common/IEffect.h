#pragma once


#include <IConverterModule.h>

X_NAMESPACE_BEGIN(engine)

namespace fx
{

	static const uint32_t	EFFECT_VERSION = 1;
	static const uint32_t	EFFECT_FOURCC = X_TAG('x', 'e', 'f', 'x');
	static const char*		EFFECT_FILE_EXTENSION = "efx";

	static const uint32_t	EFFECT_MAX_LOADED = 1 << 12;
	static const uint32_t	EFFECT_MAX_ELEMENTS = 1 << 8;



	struct IFxLib : public IConverter
	{

	};



} // namespace fx

X_NAMESPACE_END