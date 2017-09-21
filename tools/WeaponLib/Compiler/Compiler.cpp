#include "stdafx.h"
#include "Compiler.h"
#include "Util\WeaponUtil.h"

#include <String\Json.h>
#include <IFileSys.h>


X_NAMESPACE_BEGIN(game)

namespace weapon
{

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
		std::array<std::pair<const char*, core::json::Type>, 23 + StateTimer::ENUM_COUNT
			> requiredValues = { {
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

			// model
			{ "modelGun", core::json::kStringType },
			{ "modelWorld", core::json::kStringType },

			// anims
			{ "animIdle", core::json::kStringType },
			{ "animFire", core::json::kStringType },
			{ "animRaise", core::json::kStringType },
			{ "animFirstRaise", core::json::kStringType },
			{ "animDrop", core::json::kStringType },

			// sounds
			{ "sndPickup", core::json::kStringType },
			{ "sndAmmoPickup", core::json::kStringType },
			{ "sndFire", core::json::kStringType },
			{ "sndLastShot", core::json::kStringType },
			{ "sndEmptyFire", core::json::kStringType },

			// icons
			{ "iconHud", core::json::kStringType },

			// state timers
			{ "timeFire", core::json::kNumberType },
			{ "timeFireDelay", core::json::kNumberType },
			{ "timeMelee", core::json::kNumberType },
			{ "timeMeleeDelay", core::json::kNumberType },
			{ "timeReload", core::json::kNumberType },
			{ "timeReloadEmpty", core::json::kNumberType },
			{ "timeDrop", core::json::kNumberType },
			{ "timeRaise", core::json::kNumberType },
			{ "timeFirstRaise", core::json::kNumberType },
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

		damageMin_ = d["damageMinRange"].GetInt();
		damageMax_ = d["damageMaxRange"].GetInt();
		damageMinRange_ = d["damageMin"].GetInt();
		damageMaxRange_ = d["damageMax"].GetInt();
		damageMelee_ = d["damageMelee"].GetInt();

		// turn these into for loops?
		core::StackString<128, char> buf;

		// models
		for (uint32_t i = 0; i < ModelSlot::ENUM_COUNT; i++)
		{
			buf.setFmt("model%s", ModelSlot::ToString(i));
			modelSlots_[i] = d[buf.c_str()].GetString();
		}

		// anims
		for (uint32_t i = 0; i < AnimSlot::ENUM_COUNT; i++)
		{
			buf.setFmt("anim%s", AnimSlot::ToString(i));
			animSlots_[i] = d[buf.c_str()].GetString();
		}

		// sounds
		for (uint32_t i = 0; i < SoundSlot::ENUM_COUNT; i++)
		{
			buf.setFmt("snd%s", SoundSlot::ToString(i));
			sndSlots_[i] = d[buf.c_str()].GetString();
		}

		// icons
		for (uint32_t i = 0; i < IconSlot::ENUM_COUNT; i++)
		{
			buf.setFmt("icon%s", IconSlot::ToString(i));
			iconSlots_[i] = d[buf.c_str()].GetString();
		}

		// state timers
		for (uint32_t i = 0; i < StateTimer::ENUM_COUNT; i++)
		{
			buf.setFmt("time%s", StateTimer::ToString(i));
			stateTimers_[i] = d[buf.c_str()].GetFloat();
		}

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
		core::zero_object(hdr);

		hdr.fourCC = WEAPON_FOURCC;
		hdr.version = WEAPON_VERSION;

		hdr.wpnClass = wpnClass_;
		hdr.invType = invType_;
		hdr.fireType = fireType_;
		hdr.ammoCounterStyle = ammoCounterStyle_;

		hdr.flags = flags_;

		hdr.minDmg = damageMin_;
		hdr.maxDmg = damageMax_;
		hdr.meleeDmg = damageMelee_;
		hdr._pad2 = 0;

		hdr.minDmgRange = damageMinRange_;
		hdr.maxDmgRange = damageMaxRange_;

		core::ByteStream stream(g_WeaponLibArena);
		stream.reserve(256);

		// write all the slot strings.
		writeSlots<ModelSlot>(modelSlots_, hdr.modelSlots, stream);
		writeSlots<AnimSlot>(animSlots_, hdr.animSlots, stream);
		writeSlots<SoundSlot>(sndSlots_, hdr.sndSlots, stream);
		writeSlots<IconSlot>(iconSlots_, hdr.iconSlots, stream);

		hdr.stateTimers = stateTimers_;

		if (pFile->writeObj(hdr) != sizeof(hdr)) {
			X_ERROR("Weapon", "Failed to write weapon header");
			return false;
		}

		if (pFile->writeObj(stream.data(), stream.size()) != stream.size()) {
			X_ERROR("Weapon", "Failed to write weapon header");
			return false;
		}

		return true;
	}

	template<typename SlotEnum, size_t Num>
	bool WeaponCompiler::writeSlots(const StringArr<Num>& values,
		WeaponHdr::SlotArr<Num>& slotsOut, core::ByteStream& stream)
	{
		static_assert(Num == SlotEnum::ENUM_COUNT, "Size mismatch");
		for (uint32_t i = 0; i < SlotEnum::ENUM_COUNT; i++)
		{
			auto& name = values[i];
			if (name.isNotEmpty())
			{
				slotsOut[i] = safe_static_cast<WeaponHdr::SlotArr<Num>::value_type>(stream.size());
				stream.write(name.c_str(), core::strUtil::StringBytesIncNull(name));
			}
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

} // namespace weapon

X_NAMESPACE_END