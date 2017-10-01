#include "stdafx.h"
#include "WeaponDef.h"

#include <IFileSys.h>

X_NAMESPACE_BEGIN(game)

namespace weapon
{
	WeaponDef::WeaponDef(core::string& name) :
		AssetBase(name)
	{
		soundHashes_.fill(0);
	}

	bool WeaponDef::processData(core::XFile* pFile)
	{
		if (pFile->readObj(hdr_) != sizeof(hdr_)) {
			return false;
		}

		if (!hdr_.isValid()) {
			X_ERROR("WeaponDef", "Header is invalid: \"%s\"", getName().c_str());
			return false;
		}

		data_ = core::makeUnique<uint8_t[]>(g_gameArena, hdr_.dataSize);
		if (pFile->read(data_.get(), hdr_.dataSize) != hdr_.dataSize) {
			return false;
		}

		// build the sound event hashes.
		for (uint32_t i = 0; i < SoundSlot::ENUM_COUNT; i++)
		{
			if (hdr_.sndSlots[i] != 0)
			{
				soundHashes_[i] = sound::GetIDFromStr(getSoundSlot(static_cast<SoundSlot::Enum>(i)));
			}
		}

		return true;
	}


} // namespace weapon

X_NAMESPACE_END