#pragma once

#include <Assets\AssetBase.h>
#include <ISound.h>
#include <IEffect.h>

X_NAMESPACE_BEGIN(game)

namespace weapon
{
    class WeaponDef : public core::AssetBase
    {
        typedef std::array<sound::HashVal, SoundSlot::ENUM_COUNT> SoundHashArr;
        typedef std::array<engine::Material*, IconSlot::ENUM_COUNT> IconMaterialsArr;
        typedef std::array<anim::Anim*, AnimSlot::ENUM_COUNT> AnimArr;
        typedef std::array<engine::fx::Effect*, AnimSlot::ENUM_COUNT> EffectArr;

    public:
        WeaponDef(core::string& name);

        bool processData(core::XFile* pFile);
        void assignProps(const WeaponDef& oth);

        X_INLINE int32_t maxDmg(void) const;
        X_INLINE int32_t minDmg(void) const;
        X_INLINE int32_t meleeDmg(void) const;

        X_INLINE int32_t minDmgRange(void) const;
        X_INLINE int32_t maxDmgRange(void) const;

        X_INLINE WeaponClass::Enum wpnClass(void) const;
        X_INLINE InventoryType::Enum invType(void) const;
        X_INLINE FireType::Enum fireType(void) const;
        X_INLINE AmmoCounterStyle::Enum ammoCounterStyle(void) const;

        X_INLINE WeaponFlags getFlags(void) const;

        X_INLINE core::TimeVal stateTimer(StateTimer::Enum state) const;
        X_INLINE int32_t getAmmoSlot(AmmoSlot::Enum slot) const;
        X_INLINE anim::Anim* getAnim(AnimSlot::Enum slot) const;
        X_INLINE engine::Material* getIcon(IconSlot::Enum slot) const;
        X_INLINE engine::fx::Effect* getEffect(EffectSlot::Enum slot) const;

        X_INLINE const char* getModelSlot(ModelSlot::Enum slot) const;
        X_INLINE const char* getAnimSlot(AnimSlot::Enum slot) const;
        X_INLINE const char* getSoundSlot(SoundSlot::Enum slot) const;
        X_INLINE const char* getIconSlot(IconSlot::Enum slot) const;
        X_INLINE const char* getEffectSlot(EffectSlot::Enum slot) const;

        X_INLINE sound::HashVal getSoundSlotHash(SoundSlot::Enum slot) const;
        X_INLINE bool hasAnimSlot(AnimSlot::Enum slot) const;
        X_INLINE bool hasSoundSlot(SoundSlot::Enum slot) const;
        X_INLINE bool hasEffectSlot(EffectSlot::Enum slot) const;

    private:
        X_INLINE const char* stringForOffset(int32_t offset) const;

    private:
        core::UniquePointer<uint8_t[]> data_;

        SoundHashArr soundHashes_;
        IconMaterialsArr icons_;
        AnimArr animations_;
        EffectArr effects_;

        WeaponHdr hdr_;
    };

} // namespace weapon

X_NAMESPACE_END

#include "WeaponDef.inl"