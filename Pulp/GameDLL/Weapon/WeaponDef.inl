
X_NAMESPACE_BEGIN(game)

namespace weapon
{
    X_INLINE int32_t WeaponDef::maxDmg(void) const
    {
        return pHdr_->maxDmg;
    }

    X_INLINE int32_t WeaponDef::minDmg(void) const
    {
        return pHdr_->minDmg;
    }

    X_INLINE int32_t WeaponDef::meleeDmg(void) const
    {
        return pHdr_->meleeDmg;
    }

    X_INLINE int32_t WeaponDef::minDmgRange(void) const
    {
        return pHdr_->minDmgRange;
    }

    X_INLINE int32_t WeaponDef::maxDmgRange(void) const
    {
        return pHdr_->maxDmgRange;
    }

    X_INLINE WeaponClass::Enum WeaponDef::wpnClass(void) const
    {
        return pHdr_->wpnClass;
    }

    X_INLINE InventoryType::Enum WeaponDef::invType(void) const
    {
        return pHdr_->invType;
    }

    X_INLINE FireType::Enum WeaponDef::fireType(void) const
    {
        return pHdr_->fireType;
    }

    X_INLINE AmmoCounterStyle::Enum WeaponDef::ammoCounterStyle(void) const
    {
        return pHdr_->ammoCounterStyle;
    }

    X_INLINE WeaponFlags WeaponDef::getFlags(void) const
    {
        return pHdr_->flags;
    }

    X_INLINE core::TimeVal WeaponDef::stateTimer(StateTimer::Enum state) const
    {
        return core::TimeVal(pHdr_->stateTimers[state]);
    }

    X_INLINE int32_t WeaponDef::getAmmoSlot(AmmoSlot::Enum slot) const
    {
        return pHdr_->ammoSlots[slot];
    }

    X_INLINE anim::Anim* WeaponDef::getAnim(AnimSlot::Enum slot) const
    {
        return animations_[slot];
    }

    X_INLINE engine::Material* WeaponDef::getIcon(IconSlot::Enum slot) const
    {
        return icons_[slot];
    }

    X_INLINE engine::fx::Effect* WeaponDef::getEffect(EffectSlot::Enum slot) const
    {
        return effects_[slot];
    }

    X_INLINE const char* WeaponDef::getModelSlot(ModelSlot::Enum slot) const
    {
        return stringForOffset(pHdr_->modelSlots[slot]);
    }

    X_INLINE const char* WeaponDef::getAnimSlot(AnimSlot::Enum slot) const
    {
        return stringForOffset(pHdr_->animSlots[slot]);
    }

    X_INLINE const char* WeaponDef::getSoundSlot(SoundSlot::Enum slot) const
    {
        return stringForOffset(pHdr_->sndSlots[slot]);
    }

    X_INLINE const char* WeaponDef::getIconSlot(IconSlot::Enum slot) const
    {
        return stringForOffset(pHdr_->iconSlots[slot]);
    }

    X_INLINE const char* WeaponDef::getEffectSlot(EffectSlot::Enum slot) const
    {
        return stringForOffset(pHdr_->effectSlots[slot]);
    }

    X_INLINE const char* WeaponDef::getStrSlot(StringSlot::Enum slot) const
    {
        return stringForOffset(pHdr_->strSlots[slot]);
    }


    X_INLINE sound::HashVal WeaponDef::getSoundSlotHash(SoundSlot::Enum slot) const
    {
        return soundHashes_[slot];
    }

    X_INLINE const char* WeaponDef::stringForOffset(int32_t offset) const
    {
        return reinterpret_cast<const char*>(pHdr_ + 1) + offset;
    }

    X_INLINE bool WeaponDef::hasAnimSlot(AnimSlot::Enum slot) const
    {
        return pHdr_->animSlots[slot] != 0;
    }

    X_INLINE bool WeaponDef::hasSoundSlot(SoundSlot::Enum slot) const
    {
        return pHdr_->sndSlots[slot] != 0;
    }

    X_INLINE bool WeaponDef::hasEffectSlot(EffectSlot::Enum slot) const
    {
        return pHdr_->effectSlots[slot] != 0;
    }

    X_INLINE void WeaponDef::setAmmoTypeId(AmmoTypeId id)
    {
        ammoTypeId_ = id;
    }

    X_INLINE AmmoTypeId WeaponDef::getAmmoTypeId(void) const
    {
        return ammoTypeId_;
    }

} // namespace weapon

X_NAMESPACE_END
