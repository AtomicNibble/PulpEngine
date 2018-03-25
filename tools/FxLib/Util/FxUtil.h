#pragma once


X_NAMESPACE_BEGIN(engine)

namespace fx
{
	namespace Util
	{
		FXLIB_EXPORT StageType::Enum TypeFromStr(const char* pBegin, const char* pEnd);
		FXLIB_EXPORT RelativeTo::Enum RelativeToFromStr(const char* pBegin, const char* pEnd);


	} // namespace Util
} // namespace fx

X_NAMESPACE_END
