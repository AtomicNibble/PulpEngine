#include "stdafx.h"
#include "Compiler.h"

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
	std::array<std::pair<const char*, core::json::Type>, 14> requiredValues = { {
		{ "displayName", core::json::kStringType },
		{ "class", core::json::kStringType },
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



	return true;
}

bool WeaponCompiler::writeToFile(core::XFile* pFile) const
{
	X_UNUSED(pFile);

	return false;
}

X_NAMESPACE_END