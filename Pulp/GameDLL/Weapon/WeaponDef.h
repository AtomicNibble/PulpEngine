#pragma once

#include <Assets\AssetBase.h>

X_NAMESPACE_BEGIN(game)

namespace weapon
{


	class WeaponDef : public core::AssetBase
	{
	public:
		WeaponDef(core::string& name);

		bool processData(core::XFile* pFile);

		X_INLINE int32_t maxDmg(void) const;
		X_INLINE int32_t minDmg(void) const;
		X_INLINE int32_t meleeDmg(void) const;

		X_INLINE int32_t minDmgRange(void) const;
		X_INLINE int32_t maxnDmgRange(void) const;


		X_INLINE WeaponClass::Enum wpnClass(void) const;
		X_INLINE InventoryType::Enum invType(void) const;
		X_INLINE FireType::Enum fireType(void) const;
		X_INLINE AmmoCounterStyle::Enum ammoCounterStyle(void) const;

		X_INLINE WeaponFlags getFlags(void) const;

		X_INLINE core::TimeVal stateTimer(StateTimer::Enum state) const;

	private:
		core::UniquePointer<uint8_t[]> data_;

		WeaponHdr hdr_;
	};


} // namespace weapon

X_NAMESPACE_END

#include "WeaponDef.inl"