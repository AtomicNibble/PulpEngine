#include "stdafx.h"
#include "Compiler.h"
#include "Util\WeaponUtil.h"

#include <String\Json.h>
#include <IFileSys.h>

X_NAMESPACE_BEGIN(game)

namespace weapon
{
    namespace
    {
        template<typename SlotType>
        void getJsonFloat(SlotType& slot, const core::json::GenericValue<core::json::UTF8<>>& val)
        {
            slot = val.GetFloat();
        }
        template<typename SlotType>
        void getJsonString(SlotType& slot, const core::json::GenericValue<core::json::UTF8<>>& val)
        {
            slot = val.GetString();
        }
    } // namespace

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
        std::array<std::pair<const char*, core::json::Type>, 11> requiredValues = {{
            {"displayName", core::json::kStringType},
            {"class", core::json::kStringType},
            {"invType", core::json::kStringType},
            {"fireType", core::json::kStringType},
            {"ammoCounterStyle", core::json::kStringType},
            {"ammoName", core::json::kStringType},

            {"damageMinRange", core::json::kNumberType},
            {"damageMaxRange", core::json::kNumberType},
            {"damageMin", core::json::kNumberType},
            {"damageMax", core::json::kNumberType},
            {"damageMelee", core::json::kNumberType},
        }};

        for (size_t i = 0; i < requiredValues.size(); i++) {
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
        const char* pAmmoCounterStyle = d["ammoCounterStyle"].GetString();
        const char* pAmmoName = d["ammoName"].GetString();

        wpnClass_ = Util::WeaponClassFromStr(pClass);
        invType_ = Util::InventoryTypeFromStr(pInvType);
        fireType_ = Util::FireTypeFromStr(pFireType);
        ammoCounterStyle_ = Util::AmmoCounterStyleFromStr(pAmmoCounterStyle);

        damageMin_ = d["damageMin"].GetInt();
        damageMax_ = d["damageMax"].GetInt();
        damageMinRange_ = d["damageMinRange"].GetInt();
        damageMaxRange_ = d["damageMaxRange"].GetInt();
        damageMelee_ = d["damageMelee"].GetInt();

        if (core::strUtil::strlen(pAmmoName) == 0) {
            X_ERROR("Weapon", "Invalid ammo name");
            return false;
        }

        auto assignString = [](core::string& slot, core::json::GenericValue<core::json::UTF8<>>& val) {
            slot = val.GetString();
        };
        auto assignFloat = [](float& slot, core::json::GenericValue<core::json::UTF8<>>& val) {
            slot = val.GetFloat();
        };
        auto assignInt16 = [](uint16_t& slot, core::json::GenericValue<core::json::UTF8<>>& val) {
            slot = safe_static_cast<uint16_t>(val.GetInt());
        };

        // slots.
        parseEnum<ModelSlot>(d, core::json::kStringType, "model", modelSlots_, assignString);
        parseEnum<AnimSlot>(d, core::json::kStringType, "anim", animSlots_, assignString);
        parseEnum<SoundSlot>(d, core::json::kStringType, "snd", sndSlots_, assignString);
        parseEnum<IconSlot>(d, core::json::kStringType, "icon", iconSlots_, assignString);
        parseEnum<EffectSlot>(d, core::json::kStringType, "efx", effectSlots_, assignString);
        parseEnum<AmmoSlot>(d, core::json::kNumberType, "ammo", ammoSlots_, assignInt16);

        // timers
        parseEnum<StateTimer>(d, core::json::kNumberType, "time", stateTimers_, assignFloat);

        static_assert(WeaponFlag::FLAGS_COUNT == 8, "Added additional weapon flags? this code might need updating.");

        std::array<std::pair<const char*, WeaponFlag::Enum>, 8> flags = {{
            {"f_ads", WeaponFlag::Ads},
            {"f_adsFire", WeaponFlag::AdsFire},
            {"f_adsRechamber", WeaponFlag::AdsRechamber},
            {"f_adsNoAutoReload", WeaponFlag::AdsNoAutoReload},
            {"f_noPartialReload", WeaponFlag::NoPartialReload},
            {"f_noprone", WeaponFlag::NoProne},
            {"f_SegmentedReload", WeaponFlag::SegmentedReload},
            {"f_ArmorPiercing", WeaponFlag::ArmorPiercing},
        }};

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
        stream.write('\0'); // anything that's not set will just point to this null term.

        // write all the slot strings.
        writeSlots<ModelSlot>(modelSlots_, hdr.modelSlots, stream);
        writeSlots<AnimSlot>(animSlots_, hdr.animSlots, stream);
        writeSlots<SoundSlot>(sndSlots_, hdr.sndSlots, stream);
        writeSlots<IconSlot>(iconSlots_, hdr.iconSlots, stream);
        writeSlots<EffectSlot>(effectSlots_, hdr.effectSlots, stream);

        hdr.ammoSlots = ammoSlots_;
        hdr.stateTimers = stateTimers_;
        hdr.dataSize = safe_static_cast<uint16_t>(stream.size());

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

    template<typename Enum, typename SlotType, size_t SlotNum, typename Fn>
    bool WeaponCompiler::parseEnum(core::json::Document& d, core::json::Type expectedType,
        const char* pPrefix, std::array<SlotType, SlotNum>& slots, Fn callBack)
    {
        core::StackString<128, char> buf;

        for (uint32_t i = 0; i < Enum::ENUM_COUNT; i++) {
            buf.setFmt("%s%s", pPrefix, Enum::ToString(i));

            if (!d.HasMember(buf.c_str())) {
                X_ERROR("Weapon", "Missing required value: \"%s\"", buf.c_str());
                return false;
            }

            auto& m = d[buf.c_str()];
            if (m.GetType() != expectedType) {
                X_ERROR("Weapon", "Incorrect type for \"%s\"", buf.c_str());
                return false;
            }

            callBack(slots[i], m);
        }

        return true;
    }

    template<typename SlotEnum, size_t Num>
    bool WeaponCompiler::writeSlots(const StringArr<Num>& values,
        WeaponHdr::SlotArr<Num>& slotsOut, core::ByteStream& stream)
    {
        static_assert(Num == SlotEnum::ENUM_COUNT, "Size mismatch");
        for (uint32_t i = 0; i < SlotEnum::ENUM_COUNT; i++) {
            auto& name = values[i];
            if (name.isNotEmpty()) {
                slotsOut[i] = safe_static_cast<typename WeaponHdr::SlotArr<Num>::value_type>(stream.size());
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
        for (size_t i = 0; i < flagValues.size(); i++) {
            const auto& flag = flagValues[i];

            if (d.HasMember(flag.first)) {
                const auto& val = d[flagValues[i].first];

                switch (val.GetType()) {
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