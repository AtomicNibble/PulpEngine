#pragma once


#include <IMaterial.h>

X_NAMESPACE_BEGIN(engine)

namespace Util
{

	MATLIB_EXPORT MaterialMountType::Enum MatMountTypeFromStr(const char* str);
	MATLIB_EXPORT MaterialCat::Enum MatCatFromStr(const char* str);
	MATLIB_EXPORT MaterialUsage::Enum MatUsageFromStr(const char* str);
	MATLIB_EXPORT MaterialSurType::Enum MatSurfaceTypeFromStr(const char* str);
	MATLIB_EXPORT MaterialPolygonOffset::Enum MatPolyOffsetFromStr(const char* str);
	MATLIB_EXPORT render::FilterType::Enum FilterTypeFromStr(const char* str);
	MATLIB_EXPORT render::TexRepeat::Enum TexRepeatFromStr(const char* str);
	MATLIB_EXPORT render::CullType::Enum CullTypeFromStr(const char* str);
	MATLIB_EXPORT render::BlendType::Enum BlendTypeFromStr(const char* str);
	MATLIB_EXPORT render::BlendOp::Enum BlendOpFromStr(const char* str);

	MATLIB_EXPORT render::StencilOperation::Enum StencilOpFromStr(const char* str);
	MATLIB_EXPORT render::StencilFunc::Enum StencilFuncFromStr(const char* str);

	// engine::AUTO_TILING for auto.
	MATLIB_EXPORT int16_t TilingSizeFromStr(const char* str);




} // namespace Util

X_NAMESPACE_END