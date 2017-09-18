#pragma once

X_NAMESPACE_BEGIN(game)


X_DECLARE_ENUM(WeaponClass)(
	Pistol,
	Rifle,
	Smg,
	Mg
);

X_DECLARE_ENUM(InventoryType)(
	Primary
);

X_DECLARE_ENUM(FireType)(
	FullAuto,
	Single,
	Burst2,
	Burst3
);

X_DECLARE_ENUM(AmmoCounterStyle)(
	Magazine,
	ShortMagazine,
	ShotGun,
	Rocket,
	BeltFed
);

X_DECLARE_FLAGS(WeaponFlag)(
	Ads,
	AdsFire,
	NoProne
);


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


X_NAMESPACE_END

