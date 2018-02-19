#include "stdafx.h"
#include "WeaponUtil.h"

#include <Hashing\Fnva1Hash.h>

X_NAMESPACE_BEGIN(game)

namespace weapon
{
	namespace Util
	{
		using namespace core::Hash::Literals;

		WeaponClass::Enum WeaponClassFromStr(const char* pBegin, const char* pEnd)
		{
			static_assert(WeaponClass::ENUM_COUNT == 4, "Added additional weapon class types? this code needs updating.");

			const size_t len = (pEnd - pBegin);
			switch (core::Hash::Fnv1aHash(pBegin, len))
			{
				case "pistol"_fnv1a:
					return WeaponClass::Pistol;
				case "rifle"_fnv1a:
					return WeaponClass::Rifle;
				case "smg"_fnv1a:
					return WeaponClass::Smg;
				case "mg"_fnv1a:
					return WeaponClass::Mg;

				default:
					X_ERROR("Mtl", "Unknown weapon class type: '%.*s' (case-sen)", len, pBegin);
					return WeaponClass::Pistol;
			}
		}

		InventoryType::Enum InventoryTypeFromStr(const char* pBegin, const char* pEnd)
		{
			static_assert(InventoryType::ENUM_COUNT == 1, "Added additional weapon inventory types? this code needs updating.");

			const size_t len = (pEnd - pBegin);
			switch (core::Hash::Fnv1aHash(pBegin, len))
			{
				case "primary"_fnv1a:
					return InventoryType::Primary;

				default:
					X_ERROR("Mtl", "Unknown weapon inventory type: '%.*s' (case-sen)", len, pBegin);
					return InventoryType::Primary;
			}
		}

		FireType::Enum FireTypeFromStr(const char* pBegin, const char* pEnd)
		{
			static_assert(FireType::ENUM_COUNT == 4, "Added additional weapon fire types? this code needs updating.");

			const size_t len = (pEnd - pBegin);
			switch (core::Hash::Fnv1aHash(pBegin, len))
			{
				case "fullauto"_fnv1a:
					return FireType::FullAuto;
				case "single"_fnv1a:
					return FireType::Single;
				case "burst2"_fnv1a:
					return FireType::Burst2;
				case "burst3"_fnv1a:
					return FireType::Burst3;

				default:
					X_ERROR("Mtl", "Unknown weapon fire type: '%.*s' (case-sen)", len, pBegin);
					return FireType::FullAuto;
			}
		}

		AmmoCounterStyle::Enum AmmoCounterStyleFromStr(const char* pBegin, const char* pEnd)
		{
			static_assert(AmmoCounterStyle::ENUM_COUNT == 5, "Added additional weapon counter types? this code needs updating.");

			const size_t len = (pEnd - pBegin);
			switch (core::Hash::Fnv1aHash(pBegin, len))
			{
				case "magazine"_fnv1a:
					return AmmoCounterStyle::Magazine;
				case "shortmagazine"_fnv1a:
					return AmmoCounterStyle::ShortMagazine;
				case "shotgun"_fnv1a:
					return AmmoCounterStyle::ShotGun;
				case "rocket"_fnv1a:
					return AmmoCounterStyle::Rocket;
				case "beltfed"_fnv1a:
					return AmmoCounterStyle::BeltFed;

				default:
					X_ERROR("Mtl", "Unknown weapon counter type: '%.*s' (case-sen)", len, pBegin);
					return AmmoCounterStyle::Magazine;
			}
		}


	} // namespace Util
} // namespace weapon

X_NAMESPACE_END