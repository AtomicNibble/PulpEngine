#pragma once


#include <IMaterial.h>

X_NAMESPACE_BEGIN(engine)

namespace Util
{

	MaterialType::Enum MatTypeFromStr(const char* str);
	MaterialUsage::Enum MatUsageFromStr(const char* str);
	MaterialSurType::Enum MatSurfaceTypeFromStr(const char* str);
	MaterialFilterType::Enum MatFilterTypeFromStr(const char* str);
	MaterialTexRepeat::Enum MatTexRepeatFromStr(const char* str);
	MaterialPolygonOffset::Enum MatPolyOffsetFromStr(const char* str);
	MaterialCullType::Enum MatCullTypeFromStr(const char* str);
	MaterialBlendType::Enum MatBlendTypeFromStr(const char* str);

	StencilOperation::Enum StencilOpFromStr(const char* str);
	StencilFunc::Enum StencilFuncFromStr(const char* str);



} // namespace Util

X_NAMESPACE_END