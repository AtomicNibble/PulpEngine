#pragma once

#include <IConverterModule.h>


X_NAMESPACE_BEGIN(game)

namespace weapon
{

	static const uint32_t	 WEAPON_VERSION = 1;
	static const uint32_t	 WEAPON_FOURCC = X_TAG('w', 'p', 'n', 'b');

	static const char*		 WEAPON_FILE_EXTENSION = "wpn";
	static const char*		 WEAPON_DEFAULT_NAME = "default";

	static const uint32_t    WEAPON_MAX_LOADED = 128;


	struct IWeaponLib : public IConverter
	{

	};


	X_DECLARE_ENUM8(WeaponClass)(
		Pistol,
		Rifle,
		Smg,
		Mg
	);

	X_DECLARE_ENUM8(InventoryType)(
		Primary
	);

	X_DECLARE_ENUM8(FireType)(
		FullAuto,
		Single,
		Burst2,
		Burst3
	);

	X_DECLARE_ENUM8(AmmoCounterStyle)(
		Magazine,
		ShortMagazine,
		ShotGun,
		Rocket,
		BeltFed
	);

	X_DECLARE_FLAGS(WeaponFlag)(
		Ads,
		AdsFire,
		AdsRechamber,
		AdsNoAutoReload,
		NoPartialReload,
		NoProne,
		SegmentedReload,
		ArmorPiercing
	);

	typedef Flags<WeaponFlag> WeaponFlags;

	X_DECLARE_ENUM(ModelSlot)(
		Gun,
		World
	);

	X_DECLARE_ENUM(AnimSlot)(
		Idle,
		Fire,
		Raise,
		FirstRaise,
		Drop
	);

	X_DECLARE_ENUM(SoundSlot)(
		Pickup,
		AmmoPickUp,
		Fire,
		LastShot,
		EmptyFire
	);

	X_DECLARE_ENUM(IconSlot)(
		Hud
	);


	struct WeaponHdr
	{
		template<size_t N>
		using SlotArr = std::array<uint16_t, N>;

		// 4
		uint32_t fourCC;
		// 4
		uint8_t version;
		uint8_t _pad[3];

		// 4
		WeaponClass::Enum wpnClass;
		InventoryType::Enum invType;
		FireType::Enum fireType;
		AmmoCounterStyle::Enum ammoCounterStyle;

		// 4
		WeaponFlags flags;

		// 8
		uint16_t minDmg;
		uint16_t maxDmg;
		uint16_t meleeDmg;
		uint16_t _pad2;

		// 8
		uint32_t minDmgRange;
		uint32_t maxDmgRange;

		// these are all relative pointers to strings.
		// if zero it's not set.
		SlotArr<ModelSlot::ENUM_COUNT> modelSlots;
		SlotArr<AnimSlot::ENUM_COUNT> animSlots;
		SlotArr<SoundSlot::ENUM_COUNT> sndSlots;
		SlotArr<IconSlot::ENUM_COUNT> iconSlots;


		X_INLINE bool isValid(void) const
		{
			if (version != WEAPON_VERSION) {
				X_ERROR("Weapon", "weapon version is invalid. FileVer: %i RequiredVer: %i",
					version, WEAPON_VERSION);
			}

			return version == WEAPON_VERSION && fourCC == WEAPON_FOURCC;
		}

	};



	X_ENSURE_SIZE(WeaponClass, 1);
	X_ENSURE_SIZE(InventoryType, 1);
	X_ENSURE_SIZE(FireType, 1);
	X_ENSURE_SIZE(AmmoCounterStyle, 1);

	X_ENSURE_SIZE(WeaponHdr, 60);

} // namespae weapon

X_NAMESPACE_END

