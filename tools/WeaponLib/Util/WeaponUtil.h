#pragma once

#include <IWeapon.h>

X_NAMESPACE_BEGIN(game)

namespace weapon
{
	namespace Util
	{

		WeaponClass::Enum WeaponClassFromStr(const char* pBegin, const char* pEnd);
		InventoryType::Enum InventoryTypeFromStr(const char* pBegin, const char* pEnd);
		FireType::Enum FireTypeFromStr(const char* pBegin, const char* pEnd);
		AmmoCounterStyle::Enum AmmoCounterStyleFromStr(const char* pBegin, const char* pEnd);

		X_INLINE WeaponClass::Enum WeaponClassFromStr(const char* pStr);
		X_INLINE InventoryType::Enum InventoryTypeFromStr(const char* pStr);
		X_INLINE FireType::Enum FireTypeFromStr(const char* pStr);
		X_INLINE AmmoCounterStyle::Enum AmmoCounterStyleFromStr(const char* pStr);

	} // namespace Util
} // namespace weapon

X_NAMESPACE_END

#include "WeaponUtil.inl"