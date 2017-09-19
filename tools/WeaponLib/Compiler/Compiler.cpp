#include "stdafx.h"
#include "Compiler.h"
#include "Util\WeaponUtil.h"

#include <String\Json.h>
#include <IFileSys.h>


X_NAMESPACE_BEGIN(game)

WeaponCompiler::WeaponCompiler()
{

}

WeaponCompiler::~WeaponCompiler()
{

}


bool WeaponCompiler::loadFromJson(core::string& str)
{
	core::json::Document d;
	if (d.Parse(str.c_str(), str.length()).HasParseError()) {
		X_ERROR("Weapon", "Error parsing json");
		return false;
	}

	// find all the things.
	std::array<std::pair<const char*, core::json::Type>, 15> requiredValues = { {
		{ "displayName", core::json::kStringType },
		{ "class", core::json::kStringType },
		{ "invType", core::json::kStringType },
		{ "fireType", core::json::kStringType },
		{ "ammoCounter", core::json::kStringType },

		{ "damageMinRange", core::json::kNumberType },
		{ "damageMaxRange", core::json::kNumberType },
		{ "damageMin", core::json::kNumberType },
		{ "damageMax", core::json::kNumberType },
		{ "damageMelee", core::json::kNumberType },

		{ "sndPickup", core::json::kStringType },
		{ "sndAmmoPickup", core::json::kStringType },
		{ "sndFire", core::json::kStringType },
		{ "sndLastShot", core::json::kStringType },
		{ "sndEmptyFire", core::json::kStringType },
	} };

	for (size_t i = 0; i < requiredValues.size(); i++)
	{
		const auto& item = requiredValues[i];
		if (!d.HasMember(item.first)) {
			X_ERROR("Weapon", "Missing required value: \"%s\"", item.first);
			return false;
		}

		if (d[item.first].GetType() != item.second) {
			X_ERROR("Weapon", "Incorrect type for \"%s\"", item.first);
			return false;
		}
	}

	const char* pClass = d["class"].GetString();
	const char* pInvType = d["invType"].GetString();
	const char* pFireType = d["fireType"].GetString();
	const char* pAmmoCounter = d["ammoCounter"].GetString();

	wpnClass_ = Util::WeaponClassFromStr(pClass);
	invType_ = Util::InventoryTypeFromStr(pInvType);
	fireType_ = Util::FireTypeFromStr(pFireType);
	ammoCounterStyle_ = Util::AmmoCounterStyleFromStr(pAmmoCounter);

	static_assert(WeaponFlag::FLAGS_COUNT == 8, "Added additional weapon flags? this code might need updating.");

	std::array<std::pair<const char*, WeaponFlag::Enum>, 8> flags = { {
		{ "f_ads", WeaponFlag::Ads },
		{ "f_adsFire", WeaponFlag::AdsFire },
		{ "f_adsRechamber", WeaponFlag::AdsRechamber },
		{ "f_adsNoAutoReload", WeaponFlag::AdsNoAutoReload },
		{ "f_noPartialReload", WeaponFlag::NoPartialReload},
		{ "f_noprone", WeaponFlag::NoProne },
		{ "f_SegmentedReload", WeaponFlag::SegmentedReload },
		{ "f_ArmorPiercing", WeaponFlag::ArmorPiercing },
	} };

	if (!processFlagGroup(d, flags_, flags)) {
		X_ERROR("Weapon", "Failed to parse flags");
		return false;
	}

	return true;
}

bool WeaponCompiler::writeToFile(core::XFile* pFile) const
{
	X_UNUSED(pFile);

	WeaponHdr hdr;
	hdr.fourCC = WEAPON_FOURCC;
	hdr.version = WEAPON_VERSION;

	hdr.wpnClass = wpnClass_;
	hdr.invType = invType_;
	hdr.fireType = fireType_;
	hdr.ammoCounterStyle = ammoCounterStyle_;

	hdr.flags = flags_;

	if (pFile->writeObj(hdr) != sizeof(hdr)) {
		X_ERROR("Weapon", "Failed to write weapon header");
		return false;
	}

	return true;
}

template<typename FlagClass, size_t Num>
bool WeaponCompiler::processFlagGroup(core::json::Document& d, FlagClass& flags,
	const std::array<std::pair<const char*, typename FlagClass::Enum>, Num>& flagValues)
{
	// process all the flags.
	for (size_t i = 0; i < flagValues.size(); i++)
	{
		const auto& flag = flagValues[i];

		if (d.HasMember(flag.first))
		{
			const auto& val = d[flagValues[i].first];

			switch (val.GetType())
			{
				case core::json::kFalseType:
					// do nothing
					break;
				case core::json::kTrueType:
					flags.Set(flag.second);
					break;
				case core::json::kNumberType:
					if (val.IsBool()) {
						if (val.GetBool()) {
							flags.Set(flag.second);
						}
						break;
					}
					// fall through if not bool
				default:
					X_ERROR("Weapon", "Flag \"%s\" has a value with a incorrect type: %" PRIi32, flag.first, val.GetType());
					return false;
			}
		}
	}

	return true;
}


X_NAMESPACE_END