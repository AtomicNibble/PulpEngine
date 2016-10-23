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


} // namespace Util

X_NAMESPACE_END