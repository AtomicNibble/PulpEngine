#pragma once


#include <IMaterial.h>

X_NAMESPACE_BEGIN(engine)

namespace Util
{

	MATLIB_EXPORT MaterialMountType::Enum MatMountTypeFromStr(const char* str);
	MATLIB_EXPORT MaterialCat::Enum MatCatFromStr(const char* str);
	MATLIB_EXPORT MaterialUsage::Enum MatUsageFromStr(const char* str);
	MATLIB_EXPORT MaterialSurType::Enum MatSurfaceTypeFromStr(const char* str);
	MATLIB_EXPORT MaterialFilterType::Enum MatFilterTypeFromStr(const char* str);
	MATLIB_EXPORT MaterialTexRepeat::Enum MatTexRepeatFromStr(const char* str);
	MATLIB_EXPORT MaterialPolygonOffset::Enum MatPolyOffsetFromStr(const char* str);
	MATLIB_EXPORT MaterialCullType::Enum MatCullTypeFromStr(const char* str);
	MATLIB_EXPORT MaterialBlendType::Enum MatBlendTypeFromStr(const char* str);

	MATLIB_EXPORT StencilOperation::Enum StencilOpFromStr(const char* str);
	MATLIB_EXPORT StencilFunc::Enum StencilFuncFromStr(const char* str);

	// engine::AUTO_TILING for auto.
	MATLIB_EXPORT int16_t TilingSizeFromStr(const char* str);




} // namespace Util

X_NAMESPACE_END