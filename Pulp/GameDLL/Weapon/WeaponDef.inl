
X_NAMESPACE_BEGIN(game)

namespace weapon
{

	X_INLINE int32_t WeaponDef::maxDmg(void) const
	{
		return hdr_.maxDmg;
	}

	X_INLINE int32_t WeaponDef::minDmg(void) const
	{
		return hdr_.minDmg;
	}

	X_INLINE int32_t WeaponDef::meleeDmg(void) const
	{
		return hdr_.meleeDmg;
	}

	X_INLINE int32_t WeaponDef::minDmgRange(void) const
	{
		return hdr_.minDmgRange;
	}

	X_INLINE int32_t WeaponDef::maxnDmgRange(void) const
	{
		return hdr_.maxDmgRange;
	}


	X_INLINE WeaponClass::Enum WeaponDef::wpnClass(void) const
	{
		return hdr_.wpnClass;
	}

	X_INLINE InventoryType::Enum WeaponDef::invType(void) const
	{
		return hdr_.invType;
	}

	X_INLINE FireType::Enum WeaponDef::fireType(void) const
	{
		return hdr_.fireType;
	}

	X_INLINE AmmoCounterStyle::Enum WeaponDef::ammoCounterStyle(void) const
	{
		return hdr_.ammoCounterStyle;
	}

	X_INLINE WeaponFlags WeaponDef::getFlags(void) const
	{
		return hdr_.flags;
	}

	X_INLINE core::TimeVal WeaponDef::stateTimer(StateTimer::Enum state) const
	{
		return core::TimeVal(hdr_.stateTimers[state]);
	}

	X_INLINE int32_t WeaponDef::getAmmoSlot(AmmoSlot::Enum slot) const
	{
		return hdr_.ammoSlots[slot];
	}

	X_INLINE anim::Anim* WeaponDef::getAnim(AnimSlot::Enum slot) const
	{
		return animations_[slot];
	}

	X_INLINE engine::Material* WeaponDef::getIcon(IconSlot::Enum slot) const
	{
		return icons_[slot];
	}

	X_INLINE const char* WeaponDef::getModelSlot(ModelSlot::Enum slot) const
	{
		return stringForOffset(hdr_.modelSlots[slot]);
	}

	X_INLINE const char* WeaponDef::getAnimSlot(AnimSlot::Enum slot) const
	{
		return stringForOffset(hdr_.animSlots[slot]);
	}

	X_INLINE const char* WeaponDef::getSoundSlot(SoundSlot::Enum slot) const
	{
		return stringForOffset(hdr_.sndSlots[slot]);
	}

	X_INLINE const char* WeaponDef::getIconSlot(IconSlot::Enum slot) const
	{
		return stringForOffset(hdr_.iconSlots[slot]);
	}

	X_INLINE sound::HashVal WeaponDef::getSoundSlotHash(SoundSlot::Enum slot) const
	{
		return soundHashes_[slot];
	}

	X_INLINE const char* WeaponDef::stringForOffset(int32_t offset) const
	{
		return reinterpret_cast<const char*>(data_.ptr() + offset);
	}

	X_INLINE bool WeaponDef::hasAnimSlot(AnimSlot::Enum slot) const
	{
		return hdr_.animSlots[slot] != 0;
	}

	X_INLINE bool WeaponDef::hasSoundSlot(SoundSlot::Enum slot) const
	{
		return hdr_.sndSlots[slot] != 0;
	}


} // namespace weapon

X_NAMESPACE_END
