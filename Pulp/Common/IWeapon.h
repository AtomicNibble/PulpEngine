#pragma once

#include <IConverterModule.h>


X_NAMESPACE_BEGIN(game)

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


struct WeaponDesc
{
	uint32_t minDmgRange;
	uint32_t maxDmgRange;


	uint16_t minDmg;
	uint16_t maxDmg;
	uint16_t meleeDmg;
	uint16_t _pad;

	// models
	core::string gunModel;
	core::string worldModel;

	// animations
	core::string animIdle;
	core::string animFire;
	core::string animRaise;
	core::string animFirstRaise;
	core::string animDrop;

	// sounds.
	core::string sndPickUp;
	core::string sndAmmoPickUp;
	core::string sndFire;
	core::string sndLastShot;
	core::string sndEmtpyFire;
	core::string snd;

	// hud
	core::string hudIcon;
};


struct WeaponHdr
{
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

X_ENSURE_SIZE(WeaponHdr, 16);

X_NAMESPACE_END

