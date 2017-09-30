
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



} // namespace weapon

X_NAMESPACE_END
