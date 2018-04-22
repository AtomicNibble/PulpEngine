#pragma once

#include <IConverterModule.h>

X_NAMESPACE_BEGIN(game)

namespace weapon
{
    static const uint32_t WEAPON_VERSION = 3;
    static const uint32_t WEAPON_FOURCC = X_TAG('w', 'p', 'n', 'b');

    static const char* WEAPON_FILE_EXTENSION = "wpn";
    static const char* WEAPON_DEFAULT_NAME = "default";

    static const uint32_t WEAPON_MAX_LOADED = 64;
    static const uint32_t WEAPON_MAX_AMMO_TYPES = 32;

    typedef int32_t AmmoTypeId;

    static const AmmoTypeId INVALID_AMMO_TYPE = static_cast<AmmoTypeId>(-1);

    struct IWeaponLib : public IConverter
    {
    };

    X_DECLARE_ENUM(State)
    (
        Holstered,
        Idle,
        Reloading,
        OutOfAmmo,
        Raising,
        Lowering,

        PreFire,
        Fire);

    X_DECLARE_FLAGS(StateFlag)
    (
        HasRaised);

    typedef Flags<StateFlag> StateFlags;

    X_DECLARE_ENUM8(WeaponClass)
    (
        Pistol,
        Rifle,
        Smg,
        Mg);

    X_DECLARE_ENUM8(InventoryType)
    (
        Primary);

    X_DECLARE_ENUM8(FireType)
    (
        FullAuto,
        Single,
        Burst2,
        Burst3);

    X_DECLARE_ENUM8(AmmoCounterStyle)
    (
        Magazine,
        ShortMagazine,
        ShotGun,
        Rocket,
        BeltFed);

    X_DECLARE_FLAGS(WeaponFlag)
    (
        Ads,
        AdsFire,
        AdsRechamber,
        AdsNoAutoReload,
        NoPartialReload,
        NoProne,
        SegmentedReload,
        ArmorPiercing);

    typedef Flags<WeaponFlag> WeaponFlags;

    X_DECLARE_ENUM(StringSlot)
        (
            DisplayName,
            AmmoName);

    X_DECLARE_ENUM(ModelSlot)
    (
        Gun,
        World);

    X_DECLARE_ENUM(AnimSlot)
    (
        Idle,
        Fire,
        LastShot,
        Raise,
        FirstRaise,
        Lower,
        Reload,
        ReloadEmpty);

    X_DECLARE_ENUM(SoundSlot)
    (
        Pickup,
        AmmoPickup,
        Fire,
        LastShot,
        EmptyFire,
        Raise,
        Lower);

    X_DECLARE_ENUM(IconSlot)
    (
        Hud,
        AmmoCounter);

    X_DECLARE_ENUM(StateTimer)
    (
        Fire,
        FireDelay,
        Melee,
        MeleeDelay,
        Reload,
        ReloadEmpty,
        Lower,
        Raise,
        FirstRaise);

    X_DECLARE_ENUM(AmmoSlot)
    (
        Max,
        Start,
        ClipSize);

    X_DECLARE_ENUM(EffectSlot)
    (
        FlashView,
        FlashWorld,
        ShellEject);

    struct WeaponHdr
    {
        template<size_t N>
        using SlotArr = std::array<uint16_t, N>;

        template<size_t N>
        using FloatArr = std::array<float, N>;

        template<size_t N>
        using Int16Arr = std::array<uint16_t, N>;

        // 4
        uint32_t fourCC;
        // 4
        uint8_t version;
        uint8_t _pad[1];
        uint16_t dataSize;

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

        SlotArr<EffectSlot::ENUM_COUNT> effectSlots;

        Int16Arr<AmmoSlot::ENUM_COUNT> ammoSlots;
        FloatArr<StateTimer::ENUM_COUNT> stateTimers;

        SlotArr<StringSlot::ENUM_COUNT> strSlots;


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

    X_ENSURE_SIZE(WeaponHdr, 124);

} // namespace weapon

X_NAMESPACE_END
